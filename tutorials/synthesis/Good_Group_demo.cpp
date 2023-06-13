
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


class Voice1 : public SynthVoice {
    public:
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    // Mesh for soundwave
    Mesh mSpectrogram;
    vector<float> spectrum;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

    // Mesh for shapes
    Mesh mMesh;

    void init() override {
        const char name[] = "good_group.wav"; // CHANGE FILE NAME HERE
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

        spectrum.resize(FFT_SIZE/2+1);
        mSpectrogram.primitive(Mesh::LINE_STRIP);

        addDisc(mMesh, 0.3, 30);
        mMesh.decompress();
        mMesh.generateNormals();

        

    }

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
            

            if(stft(soundfile_buffer[idx])){
                // cout << "STFT" << endl;
                // loop through all frequency bins and scale the complex sample
                for(unsigned k = 0; k < stft.numBins(); ++k){
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // cout << spectrum[k] << endl;
                }
            }
        }

        if (player.pauseSignal) free();
    }

    void onProcess(Graphics &g) override {

        mSpectrogram.reset();
        g.meshColor();
        for(int i = 0; i < FFT_SIZE/4; i++){
            mSpectrogram.color(HSV(spectrum[i] * 50000000));
            mSpectrogram.vertex(i, spectrum[i], 0);
        }

        for(int i = 1; i <= 4; i++){
            double rand_scale_a = al::rnd::uniform(0.7, 1.3);
            double rand_scale_b = al::rnd::uniform(0.7, 1.3);
            double rand_scale_c = al::rnd::uniform(0.7, 1.3);
            mMesh.color(HSV(spectrum[FFT_SIZE/16 * i] * 30000000));
            Mesh copyMesh;
            copyMesh.copy(mMesh);
            copyMesh.scale(rand_scale_a, rand_scale_b, rand_scale_c);
            g.meshColor();
            g.pushMatrix();
            g.translate(-4 + 2 * i, -1, -10);
            g.draw(copyMesh);
            g.popMatrix();
        }


        
        g.pushMatrix();
        g.translate(-4, 0, -10);
        g.scale(100.0/FFT_SIZE, 50, 1.0);
        g.lineWidth(2);
        g.draw(mSpectrogram);
        g.popMatrix();
    }
    void onTriggerOn() override { player.setPlay(); }

    void onTriggerOff() override { 
        player.setPause();
        player.setRewind(); 
    }


};

class Voice2 : public SynthVoice {
    public:
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    // Mesh for soundwave
    Mesh mSpectrogram;
    vector<float> spectrum;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

    // Mesh for shapes
    Mesh mMesh;

    void init() override {
        const char name[] = "shield.wav"; // CHANGE FILE NAME HERE
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

        spectrum.resize(FFT_SIZE/2+1);
        mSpectrogram.primitive(Mesh::LINE_STRIP);

        addDisc(mMesh, 0.3, 30);
        mMesh.decompress();
        mMesh.generateNormals();
    }

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
            

            if(stft(soundfile_buffer[idx])){
                // cout << "STFT" << endl;
                // loop through all frequency bins and scale the complex sample
                for(unsigned k = 0; k < stft.numBins(); ++k){
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // cout << spectrum[k] << endl;
                }
            }
        }

        if (player.pauseSignal) free();
    }

    void onProcess(Graphics &g) override {

        mSpectrogram.reset();

        for(int i = 0; i < FFT_SIZE/4; i++){
            mSpectrogram.color(HSV(spectrum[i] * 50000000));
            mSpectrogram.vertex(i, spectrum[i], 0);
        }


        for(int i = 1; i <= 4; i++){
            double rand_scale_a = al::rnd::uniform(0.7, 1.3);
            double rand_scale_b = al::rnd::uniform(0.7, 1.3);
            double rand_scale_c = al::rnd::uniform(0.7, 1.3);
            mMesh.color(HSV(spectrum[FFT_SIZE/16 * i] * 30000000));
            Mesh copyMesh;
            copyMesh.copy(mMesh);
            copyMesh.scale(rand_scale_a, rand_scale_b, rand_scale_c);
            g.meshColor();
            g.pushMatrix();
            g.translate(-4 + 2 * i, -1, -10);
            g.draw(copyMesh);
            g.popMatrix();
        }

        


        g.meshColor();
        g.pushMatrix();
        g.translate(-3, 0, -10);
        g.scale(100.0/FFT_SIZE, 50, 1.0);
        g.lineWidth(2);
        g.draw(mSpectrogram);
        g.popMatrix();
    }
    void onTriggerOn() override { player.setPlay(); }

    void onTriggerOff() override { 
        player.setPause();
        player.setRewind(); 
    }


};

class Voice3 : public SynthVoice {
    public:
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    // Mesh for soundwave
    Mesh mSpectrogram;
    vector<float> spectrum;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

    // Mesh for shapes
    Mesh mMesh;

    void init() override {
        const char name[] = "ideal.wav"; // CHANGE FILE NAME HERE
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

        spectrum.resize(FFT_SIZE/2+1);
        mSpectrogram.primitive(Mesh::LINE_STRIP);

        addDisc(mMesh, 0.3, 30);
        mMesh.decompress();
        mMesh.generateNormals();
    }

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
            

            if(stft(soundfile_buffer[idx])){
                // cout << "STFT" << endl;
                // loop through all frequency bins and scale the complex sample
                for(unsigned k = 0; k < stft.numBins(); ++k){
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // cout << spectrum[k] << endl;
                }
            }
        }

        if (player.pauseSignal) free();
    }

    void onProcess(Graphics &g) override {

        mSpectrogram.reset();

        for(int i = 0; i < FFT_SIZE/4; i++){
            mSpectrogram.color(HSV(spectrum[i] * 50000000));
            mSpectrogram.vertex(i, spectrum[i], 0);
        }

        for(int i = 1; i <= 4; i++){
            double rand_scale_a = al::rnd::uniform(0.7, 1.3);
            double rand_scale_b = al::rnd::uniform(0.7, 1.3);
            double rand_scale_c = al::rnd::uniform(0.7, 1.3);
            mMesh.color(HSV(spectrum[FFT_SIZE/16 * i] * 30000000));
            Mesh copyMesh;
            copyMesh.copy(mMesh);
            copyMesh.scale(rand_scale_a, rand_scale_b, rand_scale_c);
            g.meshColor();
            g.pushMatrix();
            g.translate(-4 + 2 * i, -1, -10);
            g.draw(copyMesh);
            g.popMatrix();
        }

        g.meshColor();
        g.pushMatrix();
        g.translate(-3, 0, -10);
        g.scale(100.0/FFT_SIZE, 50, 1.0);
        // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
        g.lineWidth(2);
        g.draw(mSpectrogram);
        g.popMatrix();
    }
    void onTriggerOn() override { player.setPlay(); }

    void onTriggerOff() override { 
        player.setPause();
        player.setRewind(); 
    }


};

class Voice4 : public SynthVoice {
    public:
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    // Mesh for soundwave
    Mesh mSpectrogram;
    vector<float> spectrum;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

    // Mesh for shapes
    Mesh mMesh;

    void init() override {
        const char name[] = "talk.wav"; // CHANGE FILE NAME HERE
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

        spectrum.resize(FFT_SIZE/2+1);
        mSpectrogram.primitive(Mesh::LINE_STRIP);

        addDisc(mMesh, 0.3, 30);
        mMesh.decompress();
        mMesh.generateNormals();
    }

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
            

            if(stft(soundfile_buffer[idx])){
                // cout << "STFT" << endl;
                // loop through all frequency bins and scale the complex sample
                for(unsigned k = 0; k < stft.numBins(); ++k){
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // cout << spectrum[k] << endl;
                }
            }
        }

        if (player.pauseSignal) free();
    }

    void onProcess(Graphics &g) override {

        mSpectrogram.reset();

        for(int i = 0; i < FFT_SIZE/4; i++){
            mSpectrogram.color(HSV(spectrum[i] * 50000000));
            mSpectrogram.vertex(i, spectrum[i], 0);
        }

        for(int i = 1; i <= 4; i++){
            double rand_scale_a = al::rnd::uniform(0.7, 1.3);
            double rand_scale_b = al::rnd::uniform(0.7, 1.3);
            double rand_scale_c = al::rnd::uniform(0.7, 1.3);
            mMesh.color(HSV(spectrum[FFT_SIZE/16 * i] * 30000000));
            Mesh copyMesh;
            copyMesh.copy(mMesh);
            copyMesh.scale(rand_scale_a, rand_scale_b, rand_scale_c);
            g.meshColor();
            g.pushMatrix();
            g.translate(-4 + 2 * i, -1, -10);
            g.draw(copyMesh);
            g.popMatrix();
        }

        g.meshColor();
        g.pushMatrix();
        g.translate(-3, 0, -10);
        g.scale(100.0/FFT_SIZE, 50, 1.0);
        // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
        g.lineWidth(2);
        g.draw(mSpectrogram);
        g.popMatrix();
    }
    void onTriggerOn() override { player.setPlay(); }

    void onTriggerOff() override { 
        player.setPause();
        player.setRewind(); 
    }


};

class Voice5 : public SynthVoice {
    public:
    SoundFilePlayerTS player;

    vector<float> soundfile_buffer;

    // Mesh for soundwave
    Mesh mSpectrogram;
    vector<float> spectrum;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

    // Mesh for shapes
    Mesh mMesh;

    void init() override {
        const char name[] = "tate.wav"; // CHANGE FILE NAME HERE
        if(!player.open(name)){
          std::cerr << "File not found: " << name << std::endl;
          exit(1);
        }

        spectrum.resize(FFT_SIZE/2+1);
        mSpectrogram.primitive(Mesh::LINE_STRIP);

        addDisc(mMesh, 0.3, 30);
        mMesh.decompress();
        mMesh.generateNormals();
    }

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
            

            if(stft(soundfile_buffer[idx])){
                // cout << "STFT" << endl;
                // loop through all frequency bins and scale the complex sample
                for(unsigned k = 0; k < stft.numBins(); ++k){
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // cout << spectrum[k] << endl;
                }
            }
        }

        if (player.pauseSignal) free();
    }

    void onProcess(Graphics &g) override {

        mSpectrogram.reset();

        for(int i = 0; i < FFT_SIZE/4; i++){
            mSpectrogram.color(HSV(spectrum[i] * 50000000));
            mSpectrogram.vertex(i, spectrum[i], 0);
        }

        for(int i = 1; i <= 4; i++){
            double rand_scale_a = al::rnd::uniform(0.7, 1.3);
            double rand_scale_b = al::rnd::uniform(0.7, 1.3);
            double rand_scale_c = al::rnd::uniform(0.7, 1.3);
            mMesh.color(HSV(spectrum[FFT_SIZE/16 * i] * 30000000));
            Mesh copyMesh;
            copyMesh.copy(mMesh);
            copyMesh.scale(rand_scale_a, rand_scale_b, rand_scale_c);
            g.meshColor();
            g.pushMatrix();
            g.translate(-4 + 2 * i, -1, -10);
            g.draw(copyMesh);
            g.popMatrix();
        }

        g.meshColor();
        g.pushMatrix();
        g.translate(-3, 0, -10);
        g.scale(100.0/FFT_SIZE, 50, 1.0);
        // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
        g.lineWidth(2);
        g.draw(mSpectrogram);
        g.popMatrix();
    }
    void onTriggerOn() override { player.setPlay(); }

    void onTriggerOff() override { 
        player.setPause();
        player.setRewind(); 
    }


};

class MyApp : public App {
    public:
    SynthGUIManager<Voice1> synthManager {"Voice"};

    void onCreate() override {
        imguiInit();
        
        navControl().active(false);

        synthManager.synthRecorder().verbose(true);
    }

    void onSound(AudioIOData& io) override {
        synthManager.render(io); // Render audio
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

    void playVoice1(){
        auto *voice = synthManager.synth().getVoice<Voice1>();
        synthManager.synthSequencer().addVoiceFromNow(voice, 0, 39);
    }

    void playVoice2(){
        auto *voice = synthManager.synth().getVoice<Voice2>();
        synthManager.synthSequencer().addVoiceFromNow(voice, 0, 29);
    }

    void playVoice3(){
        auto *voice = synthManager.synth().getVoice<Voice3>();
        synthManager.synthSequencer().addVoiceFromNow(voice, 0, 21);
    }

    void playVoice4(){
        auto *voice = synthManager.synth().getVoice<Voice4>();
        synthManager.synthSequencer().addVoiceFromNow(voice, 0, 26);
    }

    void playVoice5(){
        auto *voice = synthManager.synth().getVoice<Voice5>();
        synthManager.synthSequencer().addVoiceFromNow(voice, 0, 56);
    }

    bool onKeyDown(Keyboard const& k) override {
        switch(k.key()){
            case 'a':
                playVoice1();
                break;
            case 's':
                playVoice2();
                break;
            case 'd':
                playVoice3();
                break;
            case 'f':
                playVoice4();
                break;
            case 'g':
                playVoice5();
                break;
            default:
                break;
        }
        return true;
    }
};

int main() {
    MyApp app;

    app.configureAudio(48000., 512, 2, 0);

    app.start();
}