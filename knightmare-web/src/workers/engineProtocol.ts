export type InitializeEngineRequest = {
  type: 'initialize'
}

export type SearchEngineRequest = {
  type: 'search'
  requestId: number
  fen: string
  depth: number
}

export type EngineWorkerRequest = InitializeEngineRequest | SearchEngineRequest

export type EngineSearchResult = {
  bestMove: string
  hasBestMove: boolean
  score: number
  depth: number
  nodes: number
  completed: boolean
  interrupted: boolean
  error: string
}

export type EngineWorkerResponse =
  | { type: 'initialized' }
  | { type: 'result'; requestId: number; fen: string; result: EngineSearchResult }
  | { type: 'error'; operation: 'initialize' | 'search'; error: string; requestId?: number; fen?: string }
