import assert from 'node:assert/strict'
import createKnightmareModule from '../knightmare-web/public/engine/knightmare.js'

const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
const OPENING_FEN = 'rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2'

const module = await createKnightmareModule({
  print: () => {},
  printErr: (message) => console.error(message),
})
const engine = new module.KnightmareEngine()

function checkSearch(result, legalMoves, description, expectedDepth) {
  assert.equal(result.error, '', `${description}: ${result.error}`)
  assert.equal(result.hasBestMove, true, `${description}: expected a best move`)
  assert.ok(result.completed, `${description}: expected requested depth to complete`)
  assert.equal(result.depth, expectedDepth, `${description}: engine should complete the requested depth`)
  assert.ok(legalMoves.includes(result.bestMove), `${description}: ${result.bestMove} must be legal`)
  console.log(`${description}: ${result.bestMove}, depth ${result.depth}, ${result.nodes} nodes`)
}

assert.equal(engine.loadFen(START_FEN), true, engine.lastError())
const startingMoves = engine.legalMoves()
assert.equal(startingMoves.length, 20, 'starting position should have 20 legal moves')
for (const depth of [2, 3, 4]) {
  const startingSearch = engine.searchBestMove(depth)
  checkSearch(startingSearch, startingMoves, `Starting position depth ${depth}`, depth)
}

assert.equal(engine.loadFen(OPENING_FEN), true, engine.lastError())
const openingMoves = engine.legalMoves()
assert.ok(openingMoves.length > 0, 'valid opening FEN should have legal moves')
const openingSearch = engine.searchBestMove(3)
checkSearch(openingSearch, openingMoves, 'Valid FEN search', 3)

const beforeInvalidFen = engine.fen()
assert.equal(engine.loadFen('this is not a FEN'), false, 'malformed FEN should be rejected')
assert.ok(engine.lastError().length > 0, 'malformed FEN should report an error')
assert.equal(engine.fen(), beforeInvalidFen, 'failed FEN load must preserve the previous position')

const repeatedSearch = engine.searchBestMove(2)
checkSearch(repeatedSearch, openingMoves, 'Repeated search after invalid FEN', 2)
assert.equal(engine.fen(), beforeInvalidFen, 'search must preserve the loaded root position')

for (const invalidDepth of [0, 65]) {
  const invalidDepthResult = engine.searchBestMove(invalidDepth)
  assert.ok(invalidDepthResult.error.length > 0, `depth ${invalidDepth} should be rejected safely`)
  assert.equal(engine.fen(), beforeInvalidFen, 'invalid depth must preserve the loaded position')
}

assert.equal(engine.reset(), true, engine.lastError())
assert.equal(engine.fen(), START_FEN, 'reset should restore the starting position')
console.log('WASM integration checks passed.')
