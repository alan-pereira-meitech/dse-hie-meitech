import { FormEvent, useState } from 'react';
import { DeviceFunctionMeta } from '../types.js';

interface FunctionCardProps {
  meta: DeviceFunctionMeta;
  actionLabel: string;
  disabled: boolean;
  onRun: (meta: DeviceFunctionMeta, payload: Record<string, unknown>) => Promise<string>;
}

export function FunctionCard({ meta, actionLabel, disabled, onRun }: FunctionCardProps) {
  const [result, setResult] = useState('—');
  const [running, setRunning] = useState(false);

  const handleSubmit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    if (running) {
      return;
    }
    try {
      setRunning(true);
      const form = event.currentTarget;
      const payload: Record<string, unknown> = {};
      if (meta.inputs) {
        for (const input of meta.inputs) {
          const value = (form.elements.namedItem(input.name) as HTMLInputElement | null)?.value ?? '';
          payload[input.name] = input.type === 'number' ? Number(value) : value;
        }
      }
      const response = await onRun(meta, payload);
      setResult(response);
    } catch (error) {
      setResult(`Erro: ${(error as Error).message}`);
    } finally {
      setRunning(false);
    }
  };

  return (
    <div className="function-card">
      <div className="function-header">
        <h4>{meta.label}</h4>
        <span className="badge">{meta.kind}</span>
      </div>
      <p className="function-description">{meta.description}</p>
      <form className="function-form" data-name={meta.name} onSubmit={handleSubmit}>
        {meta.inputs && meta.inputs.length > 0 && (
          <div className="inputs">
            {meta.inputs.map((input) => (
              <label key={input.name}>
                <span>{input.label}</span>
                <input
                  name={input.name}
                  type={input.type === 'number' ? 'number' : 'text'}
                  placeholder={input.placeholder ?? ''}
                  required
                />
              </label>
            ))}
          </div>
        )}
        <button type="submit" disabled={disabled || running}>
          {running ? 'Executando...' : actionLabel}
        </button>
      </form>
      <div className="result">{result}</div>
    </div>
  );
}
