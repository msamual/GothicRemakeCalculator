import { LockDefinition, MAX_PLATES, MIN_PLATES, MIN_POSITION, MAX_POSITION } from '../models/lock.models';

export function serializeLock(definition: LockDefinition): string {
  const name = definition.name.trim() || 'my lock';
  const lines = [`Name: ${name}`, 'Rules:'];

  definition.rules.forEach((rule, index) => {
    lines.push(`${index + 1}: ${rule.trim()}`);
  });

  lines.push('Start:');
  lines.push(`[${definition.start.join(', ')}]`);
  return lines.join('\n');
}

export function parseLockText(text: string): LockDefinition {
  const lines = text
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0);

  let name = '';
  let rules: string[] = [];
  let start: number[] = [];
  let section: 'none' | 'rules' | 'start' = 'none';

  for (const line of lines) {
    if (line.startsWith('Name:')) {
      name = line.slice('Name:'.length).trim();
      continue;
    }

    if (line === 'Rules:') {
      section = 'rules';
      continue;
    }

    if (line === 'Start:') {
      section = 'start';
      continue;
    }

    if (section === 'rules') {
      const match = line.match(/^\d+\s*:\s*(.+)$/);
      if (!match) {
        throw new Error(`Неверная строка правил: ${line}`);
      }
      rules.push(match[1].trim());
      continue;
    }

    if (section === 'start') {
      const arrayMatch = line.match(/^\[(.+)\]$/);
      if (!arrayMatch) {
        throw new Error('Неверный формат стартовых позиций');
      }

      start = arrayMatch[1]
        .split(',')
        .map((value) => Number.parseInt(value.trim(), 10))
        .filter((value) => !Number.isNaN(value));
    }
  }

  if (rules.length === 0 || start.length === 0) {
    throw new Error('Не найдены правила или стартовые позиции');
  }

  if (rules.length !== start.length) {
    throw new Error('Число правил не совпадает с числом стартовых позиций');
  }

  return { name, rules, start };
}

export function validateLock(definition: LockDefinition): string | null {
  if (definition.rules.length !== definition.start.length) {
    return 'Число правил не совпадает с числом стартовых позиций';
  }

  if (definition.rules.length < MIN_PLATES || definition.rules.length > MAX_PLATES) {
    return `Количество пластин должно быть от ${MIN_PLATES} до ${MAX_PLATES}`;
  }

  for (const position of definition.start) {
    if (position < MIN_POSITION || position > MAX_POSITION) {
      return 'Стартовые позиции должны быть от 1 до 7';
    }
  }

  for (const rule of definition.rules) {
    if (!rule.trim()) {
      return 'Пустая строка правил';
    }
  }

  return null;
}

export function timesWord(count: number): string {
  const mod10 = count % 10;
  const mod100 = count % 100;
  if (mod10 === 1 && mod100 !== 11) {
    return 'раз';
  }
  if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
    return 'раза';
  }
  return 'раз';
}

export function formatInstruction(instruction: { plate: number; count: number; direction: 'left' | 'right' }): string {
  const dir = instruction.direction === 'right' ? 'вправо' : 'влево';
  return `${instruction.plate} пластина ${instruction.count} ${timesWord(instruction.count)} ${dir}`;
}
