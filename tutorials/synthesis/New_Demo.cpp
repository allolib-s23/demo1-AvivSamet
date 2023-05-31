
#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Spatial.h"
#include "Gamma/Types.h"
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/math/al_Random.hpp"
#include "al/sound/al_SoundFile.hpp"

using namespace al;
using namespace std;
#define FFT_SIZE 4048

class Kick : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Decay<> mDecay; // Added decay envelope for pitch
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>

  void init() override {
    // Intialize amplitude envelope
    // - Minimum attack (to make it thump)
    // - Short decay
    // - Maximum amplitude
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.3);
    mAmpEnv.amp(1.0);

    // Initialize pitch decay 
    mDecay.decay(0.3);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(getInternalParameterValue("frequency"));
    mPan.pos(0);
    // (removed parameter control for attack and release)

    while (io()) {
      mOsc.freqMul(mDecay()); // Multiply pitch oscillator by next decay value
      float s1 = mOsc() *  mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }

    if (mAmpEnv.done()) free();
  }

  void onTriggerOn() override { mAmpEnv.reset(); mDecay.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

/* ---------------------------------------------------------------- */

class Hihat : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>
  
  gam::Burst mBurst; // Resonant noise with exponential decay

  void init() override {
    // Initialize burst - Main freq, filter freq, duration
    mBurst = gam::Burst(20000, 15000, 0.05);

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    while (io()) {
      float s1 = mBurst();
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // Left this in because I'm not sure how to tell when a burst is done
    if (mAmpEnv.done()) free();
  }
  void onTriggerOn() override { mBurst.reset(); }
  //void onTriggerOff() override {  }
};

/* ---------------------------------------------------------------- */

class OpenHihat : public SynthVoice {
 public:
  SoundFilePlayerTS player;

  vector<float> soundfile_buffer;

  void init() override {
    // Initialize burst - Main freq, filter freq, duration
    const char name[] = "open_hihat.wav";
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    int frames = (int)io.framesPerBuffer();
    int channels = player.soundFile.channels;
    int bufferLength = frames * channels;
    if ((int)soundfile_buffer.size() < bufferLength) {
      soundfile_buffer.resize(bufferLength);
    }
    player.getFrames(frames, soundfile_buffer.data(), (int)soundfile_buffer.size());
    int second = (channels < 2) ? 0 : 1;
    while (io()) {
      int frame = (int)io.frame();
      int idx = frame * channels;
      io.out(0) = soundfile_buffer[idx];
      io.out(1) = soundfile_buffer[idx + second];
    }
    
  }
  void onTriggerOn() override { player.setPlay(); }
  void onTriggerOff() override { 
    player.setPause();
    player.setRewind(); 
    }
};

/* ---------------------------------------------------------------- */

class Snare : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Amplitude envelope
  gam::Sine<> mOsc; // Main pitch osc (top of drum)
  gam::Sine<> mOsc2; // Secondary pitch osc (bottom of drum)
  gam::Decay<> mDecay; // Pitch decay for oscillators
//   gam::ReverbMS<> reverb;	// Schroeder reverberator
  gam::Burst mBurst; // Noise to simulate rattle/chains

  Mesh snareMesh;


  void init() override {
    // Initialize burst 
    mBurst = gam::Burst(10000, 5000, 0.3);

    // Initialize amplitude envelope
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.01);
    mAmpEnv.amp(1.0);

    // Initialize pitch decay 
    mDecay.decay(0.8);

    addRect(snareMesh, 0.3f, 0.5f);
    // reverb.resize(gam::FREEVERB);
	// 	reverb.decay(0.5); // Set decay length, in seconds
	// 	reverb.damping(0.2); // Set high-frequency damping factor in [0, 1]

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(200);
    mOsc2.freq(150);

    while (io()) {
      
      // Each mDecay() call moves it forward (I think), so we only want
      // to call it once per sample
      float decay = mDecay();
      mOsc.freqMul(decay);
      mOsc2.freqMul(decay);

      float amp = mAmpEnv();
      float s1 = mBurst() + (mOsc() * amp * 0.1)+ (mOsc2() * amp * 0.05);
    //   s1 += reverb(s1) * 0.2;
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    
    if (mAmpEnv.done()) free();
  }

  void onProcess(Graphics&g) override {
    g.pushMatrix();

    g.translate(0, -1, -4);
    g.scale(1,1,1);
    g.color(Color(0.5, 0.5, 0.5)); 
    g.draw(snareMesh);
    g.popMatrix();
  }
  void onTriggerOn() override { mBurst.reset(); mAmpEnv.reset(); mDecay.reset();}
  
  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

class Lead : public SynthVoice
{
public:
    // Unit generators
    double a = 0;
    double b = 0;
    double timepose = 0;
    Vec3f note_position;
    Vec3f note_direction;

    gam::Pan<> mPan;
    // const int static MAX_PARTIALS = 10;
    // gam::Sine<> mOsc[MAX_PARTIALS];
    gam::Sine<> mOsc;
    gam::Square<> mSquare1;
    gam::Square<> mSquare2;
    gam::Saw<> mSaw1;
    gam::Saw<> mSaw2;
    gam::Env<3> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;

    Mesh mMesh;

    // Initialize voice. This function will only be called once per voice when
    // it is created. Voices will be reused if they are idle.
    void init() override
    {
        // Intialize envelope
        mAmpEnv.curve(0); // make segments lines
        mAmpEnv.levels(0, 1, 1, 0);
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

        addSphere(mMesh, 1, 100, 100); // Add a rectangle to the mesh
        mMesh.decompress();
        mMesh.generateNormals();
        // This is a quick way to create parameters for the voice. Trigger
        // parameters are meant to be set only when the voice starts, i.e. they
        // are expected to be constant within a voice instance. (You can actually
        // change them while you are prototyping, but their changes will only be
        // stored and aplied when a note is triggered.)

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
        // createInternalTriggerParameter("partials", 3.0, 1.0, MAX_PARTIALS);
        createInternalTriggerParameter("deltaA", 0.23, -1, 1);
        createInternalTriggerParameter("deltaB", 0.29, -1, 1);
    }

    // The audio processing function
    void onProcess(AudioIOData& io) override
    {
        // Get the values from the parameters and apply them to the corresponding
        // unit generators. You could place these lines in the onTrigger() function,
        // but placing them here allows for realtime prototyping on a running
        // voice, rather than having to trigger a new voice to hear the changes.
        // Parameters will update values once per audio callback because they
        // are outside the sample processing loop.
        double freq = getInternalParameterValue("frequency");
        mSquare1.freq(freq);
        mSquare2.freq(freq * 2.0);
        // mOsc.freq(freq * 2.0);
        // int partials = (int) floor(getInternalParameterValue("partials"));
        // for(int i = 0; i < partials; i++){
        //     mOsc[i].freq(i * freq);
        // }
        
        mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
        mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
        mPan.pos(getInternalParameterValue("pan"));
        while (io())
        {
            float s1 = (mSquare1() + mSquare2() * 0.5) * mAmpEnv() * getInternalParameterValue("amplitude");
            // for(int i = 0; i < partials; i++){
            //     s1 += (1.0/(float)(i+1)) * mOsc[i]() * mAmpEnv() * getInternalParameterValue("amplitude");
            // }
            float s2;
            mPan(s1, s1, s2);
            mEnvFollow(s1);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        // We need to let the synth know that this voice is done
        // by calling the free(). This takes the voice out of the
        // rendering chain
        if (mAmpEnv.done())
            free();
    }

    // The graphics processing function
    void onProcess(Graphics& g) override
    {
        // empty if there are no graphics to draw
        a += getInternalParameterValue("deltaA");
        b += getInternalParameterValue("deltaB");
        timepose += 0.02;
        // Get the paramter values on every video frame, to apply changes to the
        // current instance
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");


        g.pushMatrix();
        g.depthTesting(true);
        g.lighting(true);
        g.translate(note_position);

        g.rotate(a, Vec3f(1, 0, 0));
        g.rotate(b, Vec3f(1));
        g.scale(0.3 + mAmpEnv() * 0.2, 0.3 + mAmpEnv() * 0.5, 1);
        g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
        g.draw(mMesh);
        g.popMatrix();
    }

    // The triggering functions just need to tell the envelope to start or release
    // The audio processing function checks when the envelope is done to remove
    // the voice from the processing chain.
    void onTriggerOn() override { 
        float frequency = getInternalParameterValue("frequency");
        float angle = frequency / 200;

        a = al::rnd::uniform();
        b = al::rnd::uniform();
        timepose = 0;
        note_position = {frequency/1000.0f - 0.5f, 0, -15};
        note_direction = {sin(angle), cos(angle), 0};
        mAmpEnv.reset(); 
    }

    void onTriggerOff() override { mAmpEnv.release(); }
};

class Pad : public SynthVoice {
  public:
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

  Mesh mSpectrogram;
  vector<float> spectrum;
  Mesh mMesh;

  void init() override {
    mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0,1,1,0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // Declare the size of the spectrum
    spectrum.resize(FFT_SIZE / 2 + 1);
    mSpectrogram.primitive(Mesh::LINE_STRIP);
    // mSpectrogram.primitive(Mesh::POINTS);

    // addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  void onProcess(AudioIOData& io) override {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    float f = getInternalParameterValue("frequency");
    mOsc.freq(f);
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while(io()){
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      mEnvFollow(s1);
      io.out(0) += s1;
      io.out(1) += s2;

      if(stft(s1)){
        // loop through all frequency bins and scale the complex sample
        for(unsigned k = 0; k < stft.numBins(); ++k){
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
        }
      }
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if(mAmpEnv.done()) free();
  }

  void onProcess(Graphics& g) override {
    // Figure out graphics for chords later

    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    mSpectrogram.reset();

    for(int i = 0; i < FFT_SIZE / 90; i++){
      mSpectrogram.color(HSV(frequency/500 - spectrum[i] * 50));
      mSpectrogram.vertex(i, spectrum[i], 0.0);
    }

    g.meshColor();
    g.pushMatrix();
    g.translate(-1.5f, 1, -10);
    g.scale(350.0/FFT_SIZE, 250, 1.0);
    // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
    g.lineWidth(1 + mEnvFollow.value() * 50);
    g.draw(mSpectrogram);
    g.popMatrix();

  }

  void onTriggerOn() override {
    mAmpEnv.reset();
  }

  void onTriggerOff() override {
    mAmpEnv.release();
  }
};


class MyApp : public App {
public:
    SynthGUIManager<Pad> synthManager {"Pad"};
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    Mesh mSpectrogram;
    vector<float> spectrum;
    
    // Time constants
    const float beat = 0.5;
    const float measure = beat * 4.0f;
    const float sixteenth = beat / 4.0f;
    const float eighth = beat / 2.0f;
    const float half = beat * 2.0f;
    const float whole = beat * 4.0f;

    // Pitch constants
    const float Bb4 = 466.16;
    const float C5 = 523.25;
    const float Db5 = 554.37;
    const float D5 = 587.33;
    const float Eb5 = 622.25;
    const float F5 = 698.46;
    const float G5 = 783.99;
    const float A5 = 880.00;
    const float Ab5 = 830.61;

    void onInit() override {
        // Set sampling rate for Gamma objects from app's audio
        
        gam::sampleRate(audioIO().framesPerSecond());
    }

    void onCreate() override {
        imguiInit();

        navControl().active(false);

        synthManager.synthRecorder().verbose(true);
    }

    void onSound(AudioIOData& io) override {
        synthManager.render(io);  // Render audio
    }

    void onAnimate(double dt) override {
        imguiBeginFrame();
        synthManager.drawSynthControlPanel();
        imguiEndFrame();
    }

    void onDraw(Graphics& g) override {
        g.clear();
        synthManager.render(g);
        imguiDraw();
    }

    void playSequenceA(){
        playRhythm(0, 16);

        playIntroMelody(0, 0.5);
        playIntroChordSequence(0, 0.5f);

        playIntroMelody(measure * 4, 0.5);
        playIntroChordSequence(measure * 4, 0.5f);

        playVerseChords(measure * 8, 0.5f);
        playVerseMelody(measure * 8, 0.5f);

        playVerseChords(measure * 10, 0.5f);
        playVerseMelody(measure * 10, 0.5f);

        playLead(Eb5 * 0.5f, measure * 12, measure);
    }

    bool onKeyDown(Keyboard const& k) override {
        switch(k.key()){
            case 'a':
                playSequenceA();
                break;
            case 's':
                player.setRewind();
                player.setPlay();
                break;
            default:
                break;
        }
        return true;
    }

    void playKick(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.01, float decay = 0.1)
  {
      auto *voice = synthManager.synth().getVoice<Kick>();
      // amp, freq, attack, release, pan
      voice->setInternalParameterValue("amp", amp);
      voice->setInternalParameterValue("attack", attack);
      voice->setInternalParameterValue("decay", decay);
      voice->setInternalParameterValue("freq", freq);
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playHihat(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Hihat>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playOpenHihat(float time, float duration)
  {
      auto *voice = synthManager.synth().getVoice<OpenHihat>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playSnare(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Snare>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }
  void playLead(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    auto* voice = synthManager.synth().getVoice<Lead>();

    voice->setInternalParameterValue("frequency", freq);
    voice->setInternalParameterValue("amplitude", amp);
    voice->setInternalParameterValue("attackTime", attack);
    voice->setInternalParameterValue("releaseTime", decay);
    voice->setInternalParameterValue("pan", 0.0f);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playPad(float freq, float offset, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    auto* voice = synthManager.synth().getVoice<Pad>();

    voice->setInternalParameterValue("frequency", freq * offset);
    voice->setInternalParameterValue("amplitude", amp);
    voice->setInternalParameterValue("attackTime", attack);
    voice->setInternalParameterValue("releaseTime", decay);
    voice->setInternalParameterValue("pan", 0.0f);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }


  void playPadChord(vector<float> freqs, float offset, float time, float duration, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    
    for(float freq : freqs){
      playPad(freq, offset, time, duration, amp, attack, decay);
    }
  }


  void playLeadChord(vector<float> freqs, float offset, float time, float duration, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    float num_notes = (float)freqs.size() - 1.0f;
    for(float freq : freqs){
      playLead(freq * offset, time, duration, amp / num_notes, attack, decay);
    }
  }



  void playRhythm(float startTime, int measureCount){
    for(int i = 0; i < measureCount; i++){
      playKick(600, startTime + i * measure, beat);
      playKick(600, startTime + i * measure + beat * 1.75f, sixteenth);
      playKick(600, startTime + i * measure + beat * 2.0f, beat);
      
      playOpenHihat(startTime + i * measure + beat * 0.5f, eighth);
      playOpenHihat(startTime + i * measure + beat * 1.5f, eighth);
      playOpenHihat(startTime + i * measure + beat * 2.5f, eighth);
      playOpenHihat(startTime + i * measure + beat * 3.5f, eighth);

      playSnare(startTime + i * measure + beat, beat);
      playSnare(startTime + i * measure + beat * 3, beat);
    }
  }

    // Possibly add an offset to the frequencies?
  void playIntroMelody(float startTime, float offset = 1.0f){
        float attack = 0.1f;
        float decay = 0.1f;
        playLead(offset * G5, startTime, beat * 3, attack, decay); 
        playLead(offset * C5 * 2, startTime + beat * 3, sixteenth, attack, decay);
        playLead(offset * Bb4 * 2,startTime +  beat * 3 + sixteenth, sixteenth, attack, decay);
        playLead(offset * F5 ,startTime +  beat * 3 + eighth, sixteenth, attack, decay);
        playLead(offset * G5, startTime + beat * 3 + eighth + sixteenth, sixteenth + beat * 3, attack, decay);

        playLead(offset * Bb4 * 2, startTime + measure + beat * 3, sixteenth, attack, decay);
        playLead(offset * G5, startTime + measure + beat * 3 + sixteenth, sixteenth, attack, decay);
        playLead(offset * Eb5, startTime + measure + beat * 3 + eighth, sixteenth, attack, decay);
        playLead(offset * F5, startTime + measure + beat * 3 + eighth + sixteenth, sixteenth + beat * 3, attack, decay);

        playLead(offset * Eb5, startTime + measure * 2 + beat * 3, sixteenth, attack, decay);
        playLead(offset * F5, startTime + measure * 2 + beat * 3 + sixteenth, sixteenth, attack, decay);
        playLead(offset * Bb4, startTime + measure * 2 + beat * 3 + eighth, sixteenth, attack, decay);  
        playLead(offset * C5, startTime + measure * 2 + beat * 3 + eighth + sixteenth, sixteenth + beat * 3, attack, decay);  
  }

  void playVerseMelody(float startTime, float offset = 1.0f){
    vector<float> combo_1 = {Bb4, G5 /2, Eb5};
    vector<float> combo_2 = {Bb4, G5 /2, D5};
    vector<float> combo_3 = {Bb4, G5 /2, Db5};
    vector<float> combo_4 = {Bb4, G5 /2, C5};

    playLeadChord(combo_1, offset, startTime, beat * 0.75);
    playLeadChord(combo_2, offset, startTime + beat * 0.75, beat * 0.75);
    playLeadChord(combo_3, offset, startTime + beat * 1.5, beat * 0.75);
    playLeadChord(combo_4, offset, startTime + beat * 2.25, beat * 1.5);
    
    playLead(Bb4 * offset, startTime + measure + beat, beat * 0.8);
    playLead((F5 / 2) * offset, startTime + measure + beat * 1.8, beat * 0.7);
    playLead((Ab5 / 2) * offset, startTime + measure + beat * 2.5, beat * 1.5);
  }

  void playIntroChordSequence(float startTime, float offset){
    vector<float> Cm7 = {C5, Eb5, G5, Bb4 * 2};
    vector<float> Fm7 = {F5, Ab5, C5, Eb5};
    vector<float> F7 = {F5, A5, C5, Eb5};
    float attack = 0.1f;
    float decay = 0.25f;

    playPadChord(Cm7, offset, startTime, measure, attack, decay);
    playPadChord(F7, offset, startTime + measure, measure, attack, decay);
    playPadChord(Fm7, offset, startTime + measure * 2, measure, attack, decay);
    playPadChord(Cm7, offset, startTime + measure * 3, measure, attack, decay);
  }

  void playVerseChords(float startTime, float offset){
    vector<float> Ebmaj7 = {Eb5, G5, Bb4 * 2, D5};
    vector<float> Bb7 = {Bb4, D5, F5, Ab5};

    playPadChord(Ebmaj7, offset, startTime, measure, 0.1f, 0.25f);
    playPadChord(Bb7, offset, startTime + measure, measure, 0.1f, 0.25f);
  }
};



int main() {
    MyApp app;

    app.configureAudio(48000., 512, 2, 0);

    app.start();
}