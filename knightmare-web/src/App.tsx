import { useCallback, useEffect, useRef, useState, type ChangeEvent } from 'react'
import { Chess, type Square } from 'chess.js'
import { Chessboard } from 'react-chessboard'
import type { ChessboardOptions } from 'react-chessboard'
import KnightmareWorker from './workers/knightmare.worker?worker'
import type { EngineWorkerResponse } from './workers/engineProtocol'
import './App.css'

type GameMode = 'local' | 'engine'
type EngineStatus = 'idle' | 'initializing' | 'ready' | 'thinking' | 'error'

const SEARCH_DEPTHS = [2, 3, 4] as const
type SearchDepth = (typeof SEARCH_DEPTHS)[number]
const DEFAULT_SEARCH_DEPTH: SearchDepth = 4

function App() {
  const [chess, setChess] = useState(() => new Chess())
  const [mode, setMode] = useState<GameMode>('local')
  const [selectedSquare, setSelectedSquare] = useState<Square | null>(null)
  const [engineStatus, setEngineStatus] = useState<EngineStatus>('idle')
  const [engineError, setEngineError] = useState('')
  const [searchDepth, setSearchDepth] = useState<SearchDepth>(DEFAULT_SEARCH_DEPTH)
  const [depthError, setDepthError] = useState('')
  const [engineAttempt, setEngineAttempt] = useState(0)
  const fenRef = useRef(chess.fen())
  const pgnRef = useRef(chess.pgn())
  const workerRef = useRef<Worker | null>(null)
  const engineReadyRef = useRef(false)
  const currentRequestIdRef = useRef(0)
  const searchDepthRef = useRef<SearchDepth>(DEFAULT_SEARCH_DEPTH)

  const history = chess.history({ verbose: true })
  const lastMove = history.at(-1)
  const legalTargets = selectedSquare
    ? chess.moves({ square: selectedSquare, verbose: true }).map((move) => move.to)
    : []

  const gameStatus = getGameStatus(chess)
  const displayStatus = mode === 'engine'
    ? getEngineDisplayStatus(engineStatus, engineError, gameStatus)
    : gameStatus
  const boardDisabled = gameStatus.isOver || (mode === 'engine' && (engineStatus !== 'ready' || chess.turn() !== 'w'))

  const commitGame = useCallback((nextGame: Chess) => {
    fenRef.current = nextGame.fen()
    pgnRef.current = nextGame.pgn()
    setChess(nextGame)
    setSelectedSquare(null)
  }, [])

  const requestEngineSearch = useCallback((fen = fenRef.current) => {
    const worker = workerRef.current
    if (!worker || !engineReadyRef.current) return
    const position = new Chess(fen)
    if (position.turn() !== 'b' || position.isGameOver()) return

    const depth = searchDepthRef.current
    if (!SEARCH_DEPTHS.includes(depth)) {
      setEngineError('Choose an engine search depth from 2 to 4.')
      setEngineStatus('error')
      return
    }

    const requestId = ++currentRequestIdRef.current
    worker.postMessage({ type: 'search', requestId, fen, depth })
    setEngineStatus('thinking')
    setEngineError('')
  }, [])

  function handleSearchDepthChange(event: ChangeEvent<HTMLSelectElement>) {
    const depth = Number(event.currentTarget.value)
    if (!SEARCH_DEPTHS.some((allowedDepth) => allowedDepth === depth)) {
      setDepthError('Choose a search depth from 2 to 4.')
      return
    }

    searchDepthRef.current = depth as SearchDepth
    setSearchDepth(depth as SearchDepth)
    setDepthError('')
  }

  useEffect(() => {
    if (mode !== 'engine') return

    const worker = new KnightmareWorker()
    workerRef.current = worker
    engineReadyRef.current = false

    worker.onmessage = (event: MessageEvent<EngineWorkerResponse>) => {
      if (workerRef.current !== worker) return
      const message = event.data
      if (message.type === 'initialized') {
        engineReadyRef.current = true
        const position = new Chess(fenRef.current)
        if (position.turn() === 'b' && !position.isGameOver()) requestEngineSearch(fenRef.current)
        else setEngineStatus('ready')
        return
      }

      if (message.type === 'error') {
        if (message.operation === 'initialize') engineReadyRef.current = false
        if (message.operation === 'search' && message.requestId !== currentRequestIdRef.current) return
        setEngineError(message.error)
        setEngineStatus('error')
        return
      }

      if (message.requestId !== currentRequestIdRef.current || message.fen !== fenRef.current) return
      if (message.result.error) {
        setEngineError(message.result.error)
        setEngineStatus('error')
        return
      }

      if (!message.result.hasBestMove) {
        const currentPosition = new Chess(fenRef.current)
        if (currentPosition.isGameOver()) {
          setEngineStatus('ready')
          return
        }
        setEngineError('Knightmare returned no move for a position that is not over.')
        setEngineStatus('error')
        return
      }

      if (!message.result.completed || !/^[a-h][1-8][a-h][1-8][qrbn]?$/.test(message.result.bestMove)) {
        setEngineError('Knightmare returned an incomplete or malformed move.')
        setEngineStatus('error')
        return
      }

      try {
        const nextGame = new Chess()
        nextGame.loadPgn(pgnRef.current)
        if (nextGame.fen() !== message.fen) return
        nextGame.move({
          from: message.result.bestMove.slice(0, 2) as Square,
          to: message.result.bestMove.slice(2, 4) as Square,
          ...(message.result.bestMove.length === 5 ? { promotion: message.result.bestMove[4] as 'q' | 'r' | 'b' | 'n' } : {}),
        })
        commitGame(nextGame)
        setEngineStatus('ready')
      } catch {
        setEngineError(`Knightmare returned an illegal move: ${message.result.bestMove}`)
        setEngineStatus('error')
      }
    }

    worker.onerror = (event) => {
      if (workerRef.current !== worker) return
      engineReadyRef.current = false
      setEngineError(event.message || 'The Knightmare worker encountered an unexpected error.')
      setEngineStatus('error')
    }
    worker.onmessageerror = () => {
      if (workerRef.current !== worker) return
      engineReadyRef.current = false
      setEngineError('The Knightmare worker sent a message that could not be read.')
      setEngineStatus('error')
    }
    worker.postMessage({ type: 'initialize' })

    return () => {
      currentRequestIdRef.current += 1
      engineReadyRef.current = false
      if (workerRef.current === worker) workerRef.current = null
      worker.terminate()
    }
  }, [mode, engineAttempt, requestEngineSearch, commitGame])

  function makeMove(from: Square, to: Square): boolean {
    if (boardDisabled) return false
    try {
      const nextGame = cloneGame(chess)
      const move = nextGame.move({ from, to, promotion: 'q' })
      if (!move) return false
      commitGame(nextGame)
      if (mode === 'engine' && nextGame.turn() === 'b' && !nextGame.isGameOver()) {
        requestEngineSearch(nextGame.fen())
      }
      return true
    } catch {
      return false
    }
  }

  function handleSquareClick(square: Square) {
    if (boardDisabled) return
    const piece = chess.get(square)

    if (selectedSquare && legalTargets.includes(square)) {
      makeMove(selectedSquare, square)
      return
    }

    if (piece?.color === chess.turn()) {
      setSelectedSquare(square)
      return
    }

    setSelectedSquare(null)
  }

  function newGame() {
    currentRequestIdRef.current += 1
    commitGame(new Chess())
    setEngineError('')
    if (mode === 'engine' && engineReadyRef.current) setEngineStatus('ready')
  }

  function undoMove() {
    if (history.length === 0) return
    const previousGame = cloneGame(chess)
    previousGame.undo()
    if (mode === 'engine' && previousGame.turn() === 'b') previousGame.undo()
    currentRequestIdRef.current += 1
    commitGame(previousGame)
    if (mode === 'engine' && engineReadyRef.current) {
      setEngineStatus('ready')
      if (previousGame.turn() === 'b' && !previousGame.isGameOver()) requestEngineSearch(previousGame.fen())
    }
  }

  function changeMode(nextMode: GameMode) {
    if (nextMode === mode) return
    currentRequestIdRef.current += 1
    setMode(nextMode)
    setEngineError('')
    setEngineStatus(nextMode === 'engine' ? 'initializing' : 'idle')
  }

  function retryEngine() {
    currentRequestIdRef.current += 1
    setEngineError('')
    setEngineStatus('initializing')
    setEngineAttempt((attempt) => attempt + 1)
  }

  const boardOptions: ChessboardOptions = {
    id: 'knightmare-board',
    position: chess.fen(),
    boardOrientation: 'white',
    allowDragging: !boardDisabled,
    showAnimations: true,
    animationDurationInMs: 180,
    darkSquareStyle: { backgroundColor: '#668579' },
    lightSquareStyle: { backgroundColor: '#e8e7d9' },
    boardStyle: { borderRadius: '8px', overflow: 'hidden' },
    squareStyles: {
      ...(lastMove ? {
        [lastMove.from]: { background: 'rgba(239, 196, 86, 0.52)' },
        [lastMove.to]: { background: 'rgba(239, 196, 86, 0.52)' },
      } : {}),
      ...(selectedSquare ? { [selectedSquare]: { background: 'rgba(239, 196, 86, 0.78)' } } : {}),
      ...Object.fromEntries(legalTargets.map((square) => [square, {
        background: chess.get(square)
          ? 'radial-gradient(transparent 56%, rgba(17, 35, 31, 0.36) 58%)'
          : 'radial-gradient(rgba(17, 35, 31, 0.28) 18%, transparent 20%)',
      }])),
    },
    onPieceDrop: ({ sourceSquare, targetSquare }) => {
      if (!targetSquare) return false
      return makeMove(sourceSquare as Square, targetSquare as Square)
    },
    onSquareClick: ({ square }) => handleSquareClick(square as Square),
    canDragPiece: ({ piece }) => !boardDisabled && piece.pieceType[0]?.toLowerCase() === chess.turn(),
  }

  const moveRows = []
  for (let i = 0; i < history.length; i += 2) {
    moveRows.push({
      number: Math.floor(i / 2) + 1,
      white: history[i],
      black: history[i + 1],
    })
  }

  return (
    <main className="app-shell">
      <header className="topbar">
        <a className="brand" href="#" aria-label="Knightmare Engine home">
          <span className="brand-mark" aria-hidden="true"><KnightMark /></span>
          <span className="brand-name">knightmare<span>engine</span></span>
        </a>
        <div className="topbar-right">
          <span className="local-badge"><span className="status-dot" /> Chess, thoughtfully played</span>
          <button className="icon-button help-button" type="button" aria-label="About Knightmare Engine" title="Knightmare Engine">
            <span>?</span>
          </button>
        </div>
      </header>

      <section className="page-intro">
        <div>
          <p className="eyebrow">THE BOARD IS YOURS</p>
          <h1>A quieter kind of <em>battle.</em></h1>
          <p className="intro-copy">Take your time. Find the move that changes everything.</p>
        </div>
        <div className="game-code"><span className="live-dot" /> GAME IN PROGRESS</div>
      </section>

      <section className="game-layout" aria-label="Chess game">
        <div className="board-column">
          <PlayerCard
            color="black"
            mode={mode}
            active={!gameStatus.isOver && (mode === 'local' ? chess.turn() === 'b' : engineStatus === 'thinking')}
            activity={mode === 'engine' ? 'THINKING' : 'TO MOVE'}
          />
          <div className={`board-frame${boardDisabled ? ' board-frame-disabled' : ''}`}>
            <Chessboard options={boardOptions} />
          </div>
          <PlayerCard
            color="white"
            mode={mode}
            active={!gameStatus.isOver && chess.turn() === 'w' && (mode === 'local' || engineStatus === 'ready')}
            activity="YOUR TURN"
          />
          <div className="board-caption">
            <span><span className="legend-dot legal-dot" /> Legal moves shown on select</span>
            <span>White to move first</span>
          </div>
        </div>

        <aside className="game-panel">
          <div className="panel-heading">
            <div>
              <p className="eyebrow">YOUR MATCH</p>
              <h2>Game room</h2>
            </div>
            <span className="room-icon" aria-hidden="true"><BoardIcon /></span>
          </div>

          <div className="mode-picker" role="group" aria-label="Game mode">
            <button className={mode === 'local' ? 'mode-option active' : 'mode-option'} onClick={() => changeMode('local')} type="button">
              <span className="mode-icon"><PeopleIcon /></span>
              <span><strong>Local play</strong><small>Two players</small></span>
              {mode === 'local' && <span className="mode-check">✓</span>}
            </button>
            <button className={mode === 'engine' ? 'mode-option active' : 'mode-option'} onClick={() => changeMode('engine')} type="button">
              <span className="mode-icon knight-icon"><KnightMark /></span>
              <span><strong>Vs Knightmare</strong><small>C++ WebAssembly engine</small></span>
              {mode === 'engine' && <span className="mode-check">✓</span>}
            </button>
          </div>

          {mode === 'engine' && (
            <div className="engine-depth-control">
              <label htmlFor="engine-search-depth">Engine Search Depth</label>
              <select
                id="engine-search-depth"
                value={searchDepth}
                onChange={handleSearchDepthChange}
                aria-invalid={depthError !== ''}
                aria-describedby={depthError ? 'engine-search-depth-error' : 'engine-search-depth-help'}
              >
                {SEARCH_DEPTHS.map((depth) => <option key={depth} value={depth}>{depth}</option>)}
              </select>
              <small id={depthError ? 'engine-search-depth-error' : 'engine-search-depth-help'} role={depthError ? 'alert' : undefined}>
                {depthError || 'Used for the next Knightmare search.'}
              </small>
            </div>
          )}

          <div className={`turn-card ${displayStatus.tone}`} aria-live="polite">
            <span className="turn-indicator" />
            <div className="turn-copy"><small>{displayStatus.label}</small><strong>{displayStatus.message}</strong></div>
            {mode === 'engine' && engineStatus === 'error' && <button className="engine-retry-button" type="button" onClick={retryEngine}>Retry</button>}
            <span className="turn-piece" aria-hidden="true">{displayStatus.piece}</span>
          </div>

          <div className="panel-actions">
            <button className="primary-button" type="button" onClick={newGame}><PlusIcon /> New game</button>
            <button className="secondary-button" type="button" onClick={undoMove} disabled={history.length === 0} title={mode === 'engine' ? 'Take back the last turn' : 'Take back the last move'}><UndoIcon /> Undo</button>
          </div>

          <section className="moves-section" aria-label="Move history">
            <div className="section-heading">
              <h3>Move history</h3>
              <span>{history.length} {history.length === 1 ? 'move' : 'moves'}</span>
            </div>
            <div className="move-list" role="log" aria-live="polite">
              {moveRows.length === 0 ? (
                <div className="empty-history">
                  <span className="empty-board-icon"><BoardIcon /></span>
                  <span>Your first move is waiting.</span>
                </div>
              ) : moveRows.map((row) => (
                <div className="move-row" key={row.number}>
                  <span className="move-number">{String(row.number).padStart(2, '0')}</span>
                  <span className="move-san">{row.white?.san}</span>
                  <span className={`move-san${row.black ? '' : ' move-pending'}`}>{row.black?.san ?? '—'}</span>
                </div>
              ))}
            </div>
          </section>

          <div className="panel-footer"><span className="footer-spark">✦</span> Good games start with good company.</div>
        </aside>
      </section>

      <footer className="site-footer">
        <span>KNIGHTMARE ENGINE <span className="footer-divider">/</span> MADE FOR THE LOVE OF THE GAME</span>
        <span className="footer-version">LOCAL PLAY · V 1.0</span>
      </footer>
    </main>
  )
}

function getGameStatus(chess: Chess) {
  if (chess.isCheckmate()) {
    const winner = chess.turn() === 'w' ? 'Black' : 'White'
    return { label: 'GAME OVER', message: `${winner} wins by checkmate`, piece: '♚', tone: 'status-win', isOver: true }
  }
  if (chess.isStalemate()) return { label: 'GAME OVER', message: 'Draw by stalemate', piece: '½', tone: 'status-draw', isOver: true }
  if (chess.isDraw()) return { label: 'GAME OVER', message: 'The game is a draw', piece: '½', tone: 'status-draw', isOver: true }
  const inCheck = chess.isCheck()
  const side = chess.turn() === 'w' ? 'White' : 'Black'
  return {
    label: inCheck ? 'CHECK' : 'CURRENT TURN',
    message: inCheck ? `${side} is in check` : `${side} to move`,
    piece: chess.turn() === 'w' ? '♔' : '♚',
    tone: inCheck ? 'status-check' : '',
    isOver: false,
  }
}

function getEngineDisplayStatus(engineStatus: EngineStatus, error: string, chessStatus: ReturnType<typeof getGameStatus>) {
  if (chessStatus.isOver) return chessStatus
  if (engineStatus === 'initializing') {
    return { label: 'INITIALIZING', message: 'Loading Knightmare engine…', piece: '♞', tone: '', isOver: false }
  }
  if (engineStatus === 'thinking') {
    return { label: 'THINKING', message: 'Knightmare is considering a move', piece: '♞', tone: '', isOver: false }
  }
  if (engineStatus === 'error') {
    return { label: 'ENGINE ERROR', message: error || 'The engine could not be started.', piece: '!', tone: 'status-error', isOver: false }
  }
  return { label: 'ENGINE READY', message: 'Your turn — White to move', piece: '♔', tone: '', isOver: false }
}

function cloneGame(chess: Chess) {
  const copy = new Chess()
  copy.loadPgn(chess.pgn())
  return copy
}

function PlayerCard({ color, active, mode, activity }: { color: 'white' | 'black'; active: boolean; mode: GameMode; activity: string }) {
  const isWhite = color === 'white'
  const name = !isWhite && mode === 'engine' ? 'Knightmare' : isWhite ? 'White' : 'Black'
  return (
    <div className={`player-card ${active ? 'player-active' : ''}`}>
      <div className={`player-avatar ${isWhite ? 'avatar-white' : 'avatar-black'}`} aria-hidden="true">{isWhite ? '♔' : '♚'}</div>
      <div className="player-info"><strong>{name} <span>{isWhite ? 'PLAYER 01' : mode === 'engine' ? 'ENGINE' : 'PLAYER 02'}</span></strong><small>{mode === 'engine' && !isWhite ? 'C++ WebAssembly engine' : 'Playing locally'}</small></div>
      {active && <span className="thinking-label"><span /> {activity}</span>}
    </div>
  )
}

function KnightMark() {
  return <svg viewBox="0 0 32 32" fill="none" aria-hidden="true"><path d="M8 26h17v-2H8v2Zm2-3h13v-2H10v2Zm1-3h10l1-5c.3-1.6-.5-3.1-1.8-4l-4-2.8 1.3-2.5-2.2-1.2-2.1 3.9 4.8 3.4c.6.4 1 1.1.8 1.8l-.5 2.4h-5.8c-1.4 0-2.5 1.1-2.5 2.5V20Zm1-1.8c0-.4.3-.7.7-.7h2.4v1.2H12v-.5ZM21 10a1.3 1.3 0 1 0 0-2.6 1.3 1.3 0 0 0 0 2.6Z" fill="currentColor" /></svg>
}

function BoardIcon() {
  return <svg viewBox="0 0 20 20" fill="none" aria-hidden="true"><rect x="2.5" y="2.5" width="15" height="15" rx="2" stroke="currentColor" strokeWidth="1.5" /><path d="M7.5 2.8v14.4M12.5 2.8v14.4M2.8 7.5h14.4M2.8 12.5h14.4" stroke="currentColor" strokeWidth="1.1" /></svg>
}

function PeopleIcon() {
  return <svg viewBox="0 0 20 20" fill="none" aria-hidden="true"><circle cx="7.5" cy="6" r="2.5" stroke="currentColor" strokeWidth="1.5" /><path d="M2.8 16v-1.3a4.7 4.7 0 0 1 9.4 0V16H2.8Zm11-10a2.3 2.3 0 0 1 0 4.5m1.1 1.6a3.8 3.8 0 0 1 2.3 3.5v.4h-2" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" /></svg>
}

function PlusIcon() {
  return <svg viewBox="0 0 20 20" fill="none" aria-hidden="true"><path d="M10 4v12M4 10h12" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" /></svg>
}

function UndoIcon() {
  return <svg viewBox="0 0 20 20" fill="none" aria-hidden="true"><path d="M7 6 3.5 9 7 12M4 9h7.3a4.2 4.2 0 1 1 0 8.4H9" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /></svg>
}

export default App
