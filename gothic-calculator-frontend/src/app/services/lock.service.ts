import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { LockDefinition, LockSolveResponse } from '../models/lock.models';

@Injectable({ providedIn: 'root' })
export class LockService {
  private readonly http = inject(HttpClient);
  private readonly apiBase = '/api/lock';

  solve(definition: LockDefinition): Observable<LockSolveResponse> {
    return this.http.post<LockSolveResponse>(`${this.apiBase}/solve`, definition);
  }
}
