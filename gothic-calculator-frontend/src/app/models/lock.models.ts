export interface LockDefinition {
  name: string;
  rules: string[];
  start: number[];
}

export interface LockInstruction {
  plate: number;
  count: number;
  direction: 'left' | 'right';
}

export interface LockSolveResponse {
  ok: boolean;
  name?: string;
  status?: 'solved' | 'already_open';
  lines?: number;
  steps?: number;
  instructions?: LockInstruction[];
  error?: string;
}

export const DEFAULT_LOCK_NAME = 'my lock';

export const TOWER_CHEST_TEMPLATE: LockDefinition = {
  name: DEFAULT_LOCK_NAME,
  rules: ['3r, 6l', '-', '1r, 4l, 6r', '2r, 5r, 6l', '-', '3l'],
  start: [5, 3, 6, 7, 2, 7],
};

export const MIN_PLATES = 2;
export const MAX_PLATES = 8;
export const MIN_POSITION = 1;
export const MAX_POSITION = 7;
export const GOAL_POSITION = 4;
