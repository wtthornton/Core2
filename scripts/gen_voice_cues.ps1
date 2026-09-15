# Regenerate Talk status WAVs (16 kHz mono 16-bit) via Windows SAPI.
# Run from repo root: powershell -File scripts/gen_voice_cues.ps1
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Speech

$dir = Join-Path (Split-Path $PSScriptRoot -Parent) "firmware\assets\cues"
New-Item -ItemType Directory -Force -Path $dir | Out-Null

$cues = @(
  @{ File = "blocked_api.wav"; Text = "Blocked. No AgentForge voice API." },
  @{ File = "spoken_blocked.wav"; Text = "Spoken blocked." },
  @{ File = "mic_missing.wav"; Text = "Microphone missing." },
  @{ File = "need_key.wav"; Text = "Need project key." },
  @{ File = "too_short.wav"; Text = "Too short." }
)
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(
  16000,
  [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,
  [System.Speech.AudioFormat.AudioChannel]::Mono)

foreach ($cue in $cues) {
  $path = Join-Path $dir $cue.File
  $synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
  $synth.Rate = -1
  $synth.Volume = 100
  $synth.SetOutputToWaveFile($path, $fmt)
  $synth.Speak($cue.Text)
  $synth.Dispose()
}

python -c @"
import array, pathlib, wave
p = pathlib.Path(r'$dir')
thresh, pad = 400, 1600
for f in sorted(p.glob('*.wav')):
    with wave.open(str(f), 'rb') as w:
        samples = array.array('h', w.readframes(w.getnframes()))
    start, end = 0, len(samples)
    while start < end and abs(samples[start]) < thresh:
        start += 1
    while end > start and abs(samples[end-1]) < thresh:
        end -= 1
    trimmed = samples[max(0, start - pad):min(len(samples), end + pad)]
    with wave.open(str(f), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(16000)
        out.writeframes(trimmed.tobytes())
    print(f'{f.name} {len(trimmed)/16000:.2f}s')
"@
