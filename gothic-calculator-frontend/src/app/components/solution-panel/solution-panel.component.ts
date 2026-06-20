import { Component, input } from '@angular/core';
import { LockSolveResponse } from '../../models/lock.models';
import { formatInstruction } from '../../utils/lock-text';

@Component({
  selector: 'app-solution-panel',
  imports: [],
  templateUrl: './solution-panel.component.html',
  styleUrl: './solution-panel.component.css',
})
export class SolutionPanelComponent {
  readonly response = input<LockSolveResponse | null>(null);
  readonly loading = input(false);
  readonly error = input<string | null>(null);

  protected formatInstruction = formatInstruction;
}
