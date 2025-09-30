# 🎤 CD+G Generator (Experimental)

This is a **basic CD+G generator** built around OpenAI Whisper and Cairo. It’s not perfect—there are bugs, quirks, and rough edges—but it works. If you want to turn an MP3 into a karaoke-style `.cdg` file with timed lyrics and visuals, this tool gets the job done.  Definite note, transcript can't start until the first second so if you have a song that starts at 0 seconds, either add a few seconds of silence or fix it so the first words start after the first second.

Zenamp loves it.

---

## 🚀 Features

- Transcribes MP3s using Whisper
- Renders each word into CD+G tile graphics
- Supports custom background images
- Outputs `.cdg` files compatible with Zenamp
- Optionally uses pre-made JSON transcripts for faster runs

---

## 🛠️ Setup

Create a virtual environment and install dependencies:

```bash
python -m venv ~/python
source ~/python/venv/bin/activate
pip install -r requirements.txt
```

🎬 Usage
Option 1: Transcribe and generate CD+G

```
python cdg.py music.mp3 music.cdg --image nerdy.jpg --json transcript.json
```

Option 2: Use existing transcript (faster)

```
python cdg.py --from-json transcript.json test.cdg --image nerdy.jpg
```

* Skips Whisper
* Uses pre-timed transcript
* Lets you edit or tweak lyrics manually

📦 Packaging for Zenamp

```
zip music.zip music.mp3 music.cdg
```

Then open music.zip in Zenamp and enjoy your DIY karaoke (you probably just wasted two hours manually editing the lyrics).


