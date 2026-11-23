'use strict';

const { spawnSync } = require('node:child_process');
const { join, resolve } = require('node:path');

const projectRoot = resolve(__dirname, '..');

function findPowerShell() {
  const candidates = process.env.STILL_AWAKE_PWSH
    ? [process.env.STILL_AWAKE_PWSH]
    : [
        process.env.USERPROFILE && join(process.env.USERPROFILE,
          '.cache', 'codex-runtimes', 'codex-primary-runtime', 'dependencies', 'native', 'powershell', 'pwsh.exe'),
        process.env.ProgramFiles && join(process.env.ProgramFiles, 'PowerShell', '7', 'pwsh.exe'),
        'pwsh.exe',
      ].filter(Boolean);

  for (const executable of [...new Set(candidates)]) {
    const probe = spawnSync(executable,
      ['-NoLogo', '-NoProfile', '-NonInteractive', '-Command', '$PSVersionTable.PSVersion.Major'],
      { encoding: 'utf8', timeout: 10000, windowsHide: true });
    if (!probe.error && probe.status === 0 && Number(probe.stdout.trim()) >= 7) {
      return executable;
    }
  }
  throw new Error('PowerShell 7 is required. Set STILL_AWAKE_PWSH to the full path of pwsh.exe.');
}

try {
  if (process.platform !== 'win32') {
    throw new Error('This command builds the Windows game and must run on Windows with Unreal Engine 5.8.2 installed.');
  }
  const options = process.argv.slice(2);
  const presets = {
    '--shipping': { task: 'Build', configuration: 'Shipping' },
    '--compile': { task: 'Compile', configuration: 'Development' },
    '--validate': { task: 'Validate', configuration: 'Development' },
  };
  if (options.length > 1 || (options.length === 1 && !presets[options[0]])) {
    throw new Error('Use npm run build, npm run build:shipping, npm run build:editor, or npm test.');
  }
  const { task, configuration } = presets[options[0]] ?? { task: 'Build', configuration: 'Development' };
  const powershell = findPowerShell();
  console.log(`Using PowerShell: ${powershell}`);
  console.log(`Still Awake: ${task} / ${configuration}. Logs and results are saved locally.`);
  const result = spawnSync(powershell,
    ['-NoLogo', '-NoProfile', '-NonInteractive', '-File', join(projectRoot, 'Build.ps1'),
      '-Task', task, '-Configuration', configuration],
    { cwd: projectRoot, stdio: 'inherit', windowsHide: true });
  if (result.error) throw result.error;
  if (result.signal) throw new Error(`Build interrupted by ${result.signal}.`);
  process.exitCode = result.status ?? 1;
} catch (error) {
  console.error(`Build failed: ${error.message}`);
  process.exitCode = 1;
}
