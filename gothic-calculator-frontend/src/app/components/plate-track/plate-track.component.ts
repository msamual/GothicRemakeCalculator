import { Component, input } from '@angular/core';
import { GOAL_POSITION, MAX_POSITION, MIN_POSITION } from '../../models/lock.models';

@Component({
  selector: 'app-plate-track',
  imports: [],
  templateUrl: './plate-track.component.html',
  styleUrl: './plate-track.component.css',
})
export class PlateTrackComponent {
  readonly plateIndex = input.required<number>();
  readonly position = input.required<number>();

  protected readonly slots = Array.from(
    { length: MAX_POSITION - MIN_POSITION + 1 },
    (_, index) => MIN_POSITION + index,
  );
  protected readonly goalPosition = GOAL_POSITION;
}
