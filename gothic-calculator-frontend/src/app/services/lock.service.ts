import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { LockDefinition, LockSolveResponse } from '../models/lock.models';

@Injectable({ providedIn: 'root' })
export class LockService {
  private readonly http = inject(HttpClient);

  solve(definition: LockDefinition): Observable<LockSolveResponse> {
    return this.http.post<LockSolveResponse>('api/lock/solve', definition);
  }
}
