import type {
  EngineSearchResult,
  EngineWorkerRequest,
  EngineWorkerResponse,
  SearchEngineRequest,
} from './engineProtocol'

type WasmEngine = {
  loadFen(fen: string): boolean
  searchBestMove(depth: number): EngineSearchResult
  lastError(): string
}

type WasmModule = {
  KnightmareEngine: new () => WasmEngine
}

type WasmModuleFactory = (options: {
  locateFile: (path: string) => string
  print: (message: string) => void
  printErr: (message: string) => void
}) => Promise<WasmModule>

let engine: WasmEngine | null = null
let initialization: Promise<void> | null = null
let searchInProgress = false

async function initializeEngine() {
  if (engine) return
  if (!initialization) {
    initialization = (async () => {
      const moduleUrl = new URL(`${import.meta.env.BASE_URL}engine/knightmare.js`, self.location.origin).href
      const imported = await import(/* @vite-ignore */ moduleUrl) as { default: WasmModuleFactory }
      const moduleFactory = imported.default
      if (typeof moduleFactory !== 'function') {
        throw new Error('The Knightmare WebAssembly module did not expose its module factory.')
      }
      const module = await moduleFactory({
        locateFile: (path) => new URL(path, moduleUrl).href,
        print: () => {},
        printErr: (message) => console.error('[Knightmare WASM]', message),
      })
      engine = new module.KnightmareEngine()
    })()
  }
  await initialization
}

function post(response: EngineWorkerResponse) {
  self.postMessage(response)
}

async function handleRequest(request: EngineWorkerRequest) {
  if (request.type === 'initialize') {
    try {
      await initializeEngine()
      post({ type: 'initialized' })
    } catch (error) {
      initialization = null
      const message = error instanceof Error ? error.message : String(error)
      post({ type: 'error', operation: 'initialize', error: message })
    }
    return
  }

  await handleSearch(request)
}

async function handleSearch(request: SearchEngineRequest) {
  try {
    if (![2, 3, 4].includes(request.depth)) {
      throw new Error('Search depth must be 2, 3, or 4 for website searches.')
    }
    await initializeEngine()
    if (searchInProgress) throw new Error('A search is already running in the engine worker.')
    if (!engine) throw new Error('The Knightmare engine did not initialize.')

    searchInProgress = true
    if (!engine.loadFen(request.fen)) {
      throw new Error(engine.lastError() || 'The engine rejected the requested FEN.')
    }

    const result = engine.searchBestMove(request.depth)
    if (result.error) throw new Error(result.error)
    post({ type: 'result', requestId: request.requestId, fen: request.fen, result })
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error)
    post({
      type: 'error',
      operation: 'search',
      requestId: request.requestId,
      fen: request.fen,
      error: message,
    })
  } finally {
    searchInProgress = false
  }
}

self.onmessage = (event: MessageEvent<EngineWorkerRequest>) => {
  void handleRequest(event.data)
}
