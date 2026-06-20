import { Component } from '@angular/core';
import { LockEditorComponent } from './components/lock-editor/lock-editor.component';

@Component({
  selector: 'app-root',
  imports: [LockEditorComponent],
  templateUrl: './app.html',
  styleUrl: './app.css',
})
export class App {}
