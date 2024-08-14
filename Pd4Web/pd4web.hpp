#pragma once

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/webaudio.h>

#include <vector>
#include <z_libpd.h>

#include "config.h"

#include <variant>

#define PD4WEB_MAJOR_VERSION 2
#define PD4WEB_MINOR_VERSION 0
#define PD4WEB_MICRO_VERSION 0

#define PD4WEB_DEBUG true

static uint8_t WasmAudioWorkletStack[1024 * 1024];

void Pd4WebInitExternals(); // defined in externals.cpp (generated by pd4web python package)

// ╭─────────────────────────────────────╮
// │            Gui Interface            │
// ╰─────────────────────────────────────╯

using ListItem = std::variant<float, std::string>;
using ItemList = std::vector<ListItem>;

struct Pd4WebGuiReceiver {
    std::string Receiver;
    enum Pd4WebGuiTypeEnum { BANG = 0, FLOAT, SYMBOL, LIST, MESSAGE } Type;
    bool BeingUpdated = false;
    bool Updated;

    float Float;
    std::string Symbol;

    ItemList List;
};

using Pd4WebGuiReceiverList = std::vector<Pd4WebGuiReceiver>;

// ╭─────────────────────────────────────╮
// │             Main Class              │
// ╰─────────────────────────────────────╯
class Pd4Web {
  public:
    static void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success,
                                             void *userData);

    // Main
    void init();
    void suspendAudio();
    void resumeAudio();
    static void post(const char *message);

    // Audio Worklets
    static EM_BOOL process(int numInputs, const AudioSampleFrame *In, int numOutputs,
                           AudioSampleFrame *Out, int numParams, const AudioParamFrame *params,
                           void *userData);
    static void audioWorkletInit(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success,
                                 void *userData);

    // Gui
    static void mainLoop();

    // Receivers
    static void AW_ReceivedBang(const char *r);
    static void AW_ReceivedFloat(const char *r, float f);
    static void AW_ReceivedSymbol(const char *r, const char *s);
    static void AW_ReceivedList(const char *r, int argc, t_atom *argv);

    // bind symbols
    void bindReceiver(std::string s);
    void addGuiReceiver(std::string s);
    void unbindReceiver();

    // midi
    void noteOn(int channel, int pitch, int velocity);

    // send Messages
    bool sendFloat(std::string r, float f);
    bool sendSymbol(std::string r, std::string s);
    bool sendBang(std::string r);

    bool _startMessage(int argc);
    void _addFloat(float f);
    void _addSymbol(std::string s);
    void _finishMessage(std::string s);

    // libpd HOOKs helpers
    int _getReceivedListSize(std::string r);
    std::string _getItemFromListType(std::string r, int i);
    std::string _getItemFromListSymbol(std::string r, int i);
    float _getItemFromListFloat(std::string r, int i);

  private:
    static int GetNInputChannels();
    static int GetNOutputChannels();
    static uint32_t GetSampleRate();
    void bindGuiReceivers();

    bool m_Pd4WebInit = false;
    EMSCRIPTEN_WEBAUDIO_T m_Context;
    bool m_PdInit = false;

    // Lib Pd
    void *m_AudioWorkletInstance;
    std::vector<std::string> m_Receivers;
};

// ╭─────────────────────────────────────╮
// │  Bind C++ functions to JavaScript   │
// ╰─────────────────────────────────────╯
EMSCRIPTEN_BINDINGS(WebPd) {
    emscripten::class_<Pd4Web>("Pd4Web")
        .constructor<>() // Default constructor
        .function("init", &Pd4Web::init)
        .function("suspendAudio", &Pd4Web::suspendAudio)
        .function("resumeAudio", &Pd4Web::resumeAudio)

        // senders
        .function("sendFloat", &Pd4Web::sendFloat)
        .function("sendSymbol", &Pd4Web::sendSymbol)
        .function("sendBang", &Pd4Web::sendBang)

        // sendList is added by _Pd4WebJSFunctions();
        .function("_startMessage", &Pd4Web::_startMessage)
        .function("_addFloat", &Pd4Web::_addFloat)
        .function("_addSymbol", &Pd4Web::_addSymbol)
        .function("_finishMessage", &Pd4Web::_finishMessage)

        // Get List
        .function("_getReceivedListSize", &Pd4Web::_getReceivedListSize)
        .function("_getItemFromListType", &Pd4Web::_getItemFromListType)
        .function("_getItemFromListSymbol", &Pd4Web::_getItemFromListSymbol)
        .function("_getItemFromListFloat", &Pd4Web::_getItemFromListFloat)

        // Midi
        .function("noteOn", &Pd4Web::noteOn)

        // bind list
        .function("bindReceiver", &Pd4Web::bindReceiver)
        .function("addGuiReceiver", &Pd4Web::addGuiReceiver)
        .function("unbindReceiver", &Pd4Web::unbindReceiver);
}
