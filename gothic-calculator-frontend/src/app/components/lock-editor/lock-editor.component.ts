import { Component, computed, inject, OnInit, signal } from '@angular/core';
import { FormArray, FormBuilder, FormControl, ReactiveFormsModule, Validators } from '@angular/forms';
import { finalize } from 'rxjs';
import { PlateTrackComponent } from '../plate-track/plate-track.component';
import { SolutionPanelComponent } from '../solution-panel/solution-panel.component';
import {
  LockDefinition,
  LockSolveResponse,
  MAX_PLATES,
  MAX_POSITION,
  MIN_PLATES,
  MIN_POSITION,
  TOWER_CHEST_TEMPLATE,
} from '../../models/lock.models';
import { LockService } from '../../services/lock.service';
import { parseLockText, serializeLock, validateLock } from '../../utils/lock-text';

@Component({
  selector: 'app-lock-editor',
  imports: [ReactiveFormsModule, PlateTrackComponent, SolutionPanelComponent],
  templateUrl: './lock-editor.component.html',
  styleUrl: './lock-editor.component.css',
})
export class LockEditorComponent implements OnInit {
  private readonly fb = inject(FormBuilder);
  private readonly lockService = inject(LockService);

  protected readonly loading = signal(false);
  protected readonly response = signal<LockSolveResponse | null>(null);
  protected readonly clientError = signal<string | null>(null);
  protected readonly showImport = signal(false);
  protected readonly importText = signal('');
  protected readonly importError = signal<string | null>(null);
  protected readonly copyMessage = signal<string | null>(null);

  protected readonly minPlates = MIN_PLATES;
  protected readonly maxPlates = MAX_PLATES;
  protected readonly minPosition = MIN_POSITION;
  protected readonly maxPosition = MAX_POSITION;

  protected readonly form = this.fb.group({
    name: this.fb.nonNullable.control('Second chest in the tower'),
    plateCount: this.fb.nonNullable.control(6, [
      Validators.min(MIN_PLATES),
      Validators.max(MAX_PLATES),
    ]),
    rules: this.fb.array<FormControl<string>>([]),
    start: this.fb.array<FormControl<number>>([]),
  });

  protected readonly positions = computed(() =>
    this.startControls.controls.map((control) => Number(control.value)),
  );

  get rulesControls(): FormArray<FormControl<string>> {
    return this.form.controls.rules;
  }

  get startControls(): FormArray<FormControl<number>> {
    return this.form.controls.start;
  }

  ngOnInit(): void {
    this.loadDefinition(TOWER_CHEST_TEMPLATE);
    this.form.controls.plateCount.valueChanges.subscribe((count) => {
      if (count !== null) {
        this.resizePlates(count);
      }
    });
  }

  protected loadTemplate(): void {
    this.loadDefinition(TOWER_CHEST_TEMPLATE);
    this.response.set(null);
    this.clientError.set(null);
  }

  protected solve(): void {
    this.clientError.set(null);
    this.response.set(null);
    this.copyMessage.set(null);

    const definition = this.currentDefinition();
    const validationError = validateLock(definition);
    if (validationError) {
      this.clientError.set(validationError);
      return;
    }

    this.loading.set(true);
    this.lockService
      .solve(definition)
      .pipe(finalize(() => this.loading.set(false)))
      .subscribe({
        next: (result) => {
          if (!result.ok) {
            this.clientError.set(result.error ?? 'Не удалось решить замок');
            return;
          }
          this.response.set(result);
        },
        error: () => {
          this.clientError.set('Не удалось связаться с сервером. Запущен ли API?');
        },
      });
  }

  protected exportText(): void {
    const text = serializeLock(this.currentDefinition());
    navigator.clipboard
      .writeText(text)
      .then(() => {
        this.copyMessage.set('Текст замка скопирован в буфер обмена');
        setTimeout(() => this.copyMessage.set(null), 2500);
      })
      .catch(() => {
        this.copyMessage.set('Не удалось скопировать текст');
      });
  }

  protected openImport(): void {
    this.importText.set(serializeLock(this.currentDefinition()));
    this.importError.set(null);
    this.showImport.set(true);
  }

  protected closeImport(): void {
    this.showImport.set(false);
  }

  protected applyImport(): void {
    try {
      const definition = parseLockText(this.importText());
      this.loadDefinition(definition);
      this.response.set(null);
      this.clientError.set(null);
      this.showImport.set(false);
    } catch (error) {
      this.importError.set(error instanceof Error ? error.message : 'Ошибка импорта');
    }
  }

  protected onImportTextChange(value: string): void {
    this.importText.set(value);
    this.importError.set(null);
  }

  private loadDefinition(definition: LockDefinition): void {
    this.resizePlates(definition.rules.length);
    this.form.patchValue(
      {
        name: definition.name,
        plateCount: definition.rules.length,
      },
      { emitEvent: false },
    );
    definition.rules.forEach((rule, index) => {
      this.rulesControls.at(index).setValue(rule);
    });
    definition.start.forEach((position, index) => {
      this.startControls.at(index).setValue(position);
    });
  }

  private resizePlates(count: number): void {
    while (this.rulesControls.length < count) {
      this.rulesControls.push(this.fb.nonNullable.control('-'));
    }
    while (this.rulesControls.length > count) {
      this.rulesControls.removeAt(this.rulesControls.length - 1);
    }

    while (this.startControls.length < count) {
      this.startControls.push(
        this.fb.nonNullable.control(4, [
          Validators.min(MIN_POSITION),
          Validators.max(MAX_POSITION),
        ]),
      );
    }
    while (this.startControls.length > count) {
      this.startControls.removeAt(this.startControls.length - 1);
    }
  }

  private currentDefinition(): LockDefinition {
    return {
      name: this.form.controls.name.value,
      rules: this.rulesControls.controls.map((control) => control.value.trim()),
      start: this.startControls.controls.map((control) => Number(control.value)),
    };
  }
}
