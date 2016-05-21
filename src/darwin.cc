#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>

#include <vector>

#include <node.h>
#include <nan.h>

static const int kNumberBuffers = 3;
static const int kBufferByteSize = 8192;

class LineIn : public Nan::ObjectWrap {
public:
  LineIn();
  ~LineIn();

  void enqueueBuffer(AudioQueueBufferRef bufferRef);
  static NAN_MODULE_INIT(Init);

private:
  static NAN_METHOD(New);
  static NAN_METHOD(Read);

  void emitQueue();
  static void _emitQueue(uv_async_t* handle);

  AudioStreamBasicDescription desc;
  AudioQueueRef audioQueue;

  bool hasEnded;
  std::vector<AudioQueueBufferRef> *bufferQueue;

  uv_async_t async;
  uv_mutex_t async_lock;
};

void callback (void *userData, AudioQueueRef audioQueue, AudioQueueBufferRef bufferRef, const AudioTimeStamp *startTime, UInt32 inumberPacketDescriptions, const AudioStreamPacketDescription *packetDescs) {
  static_cast<LineIn*>(userData)->enqueueBuffer(bufferRef);
}

LineIn::LineIn() {
  uv_async_init(uv_default_loop(), &async, _emitQueue);
  uv_mutex_init(&async_lock);

  hasEnded = false;
  async.data = static_cast<void*>(this);
  bufferQueue = new std::vector<AudioQueueBufferRef>();
}

LineIn::~LineIn() {
  hasEnded = true;

  AudioQueueStop(audioQueue, true);
  uv_mutex_destroy(&async_lock);

  delete bufferQueue;
}

void LineIn::enqueueBuffer(AudioQueueBufferRef bufferRef) {
  if (hasEnded == true) {
    AudioQueueFreeBuffer(audioQueue, bufferRef);
    return;
  }

  // Add buffer to queue
  uv_mutex_lock(&async_lock);
  bufferQueue->push_back(bufferRef);
  uv_mutex_unlock(&async_lock);

  // Schedule emitting to JS land
  uv_async_send(&async);
}

void LineIn::emitQueue() {
  Nan::HandleScope scope;

  // Fetch and reset current queue
  uv_mutex_lock(&async_lock);
  const auto processingQueue = bufferQueue;
  bufferQueue = new std::vector<AudioQueueBufferRef>();
  uv_mutex_unlock(&async_lock);

  // Get a handle to the `push` function in JS land
  auto push = Nan::Get(handle(), Nan::New("push").ToLocalChecked()).ToLocalChecked().As<v8::Function>();

  for (auto const &bufferRef : *processingQueue) {
    // Make a copy of the buffer
    auto buffer = Nan::CopyBuffer(static_cast<char*>(bufferRef->mAudioData), bufferRef->mAudioDataByteSize).ToLocalChecked();

    // Return the buffer back to the audio queue
    AudioQueueEnqueueBuffer(audioQueue, bufferRef, 0, nullptr);

    // Push the copied buffer to JS land
    Nan::TryCatch tc;
    v8::Local<v8::Value> argv[] = { buffer };
    push->Call(Nan::GetCurrentContext()->Global(), 1, argv);

    // Propagate errors from `push`
    if (tc.HasCaught()) {
      // TODO: emit error event
      Nan::FatalException(tc);
    }
  }

  // Free the old queue
  delete processingQueue;
}

void LineIn::_emitQueue(uv_async_t* handle) {
  static_cast<LineIn*>(handle->data)->emitQueue();
}

NAN_MODULE_INIT(LineIn::Init) {
  auto cname = Nan::New("LineIn").ToLocalChecked();
  auto ctor = Nan::New<v8::FunctionTemplate>(New);
  auto ctorInst = ctor->InstanceTemplate();

  ctor->SetClassName(cname);
  ctorInst->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(ctor, "_read", Read);

  Nan::Set(target, cname, Nan::GetFunction(ctor).ToLocalChecked());
}

NAN_METHOD(LineIn::New) {
  OSStatus status;

  if (!info.IsConstructCall()) {
    return Nan::ThrowTypeError("Cannot call a class constructor");
  }

  auto instance = new LineIn();
  instance->Wrap(info.This());

  instance->desc.mSampleRate = 44100;
  instance->desc.mFormatID = kAudioFormatLinearPCM;
  instance->desc.mFormatFlags = kAudioFormatFlagIsSignedInteger;
  instance->desc.mBytesPerPacket = 0;
  instance->desc.mFramesPerPacket = 1;
  instance->desc.mBytesPerFrame = 0;
  instance->desc.mChannelsPerFrame = 2;
  instance->desc.mBitsPerChannel = 16;
  instance->desc.mReserved = 0;

  status = AudioQueueNewInput(&instance->desc, callback, instance, nullptr, nullptr, 0, &instance->audioQueue);
  if (status != 0) exit(1);

  for (int i = 0; i < kNumberBuffers; ++i) {
    AudioQueueBufferRef buffer;

    status = AudioQueueAllocateBuffer(instance->audioQueue, kBufferByteSize, &buffer);
    if (status != 0) exit(3);

    status = AudioQueueEnqueueBuffer(instance->audioQueue, buffer, 0, nullptr);
    if (status != 0) exit(4);
  }
}

NAN_METHOD(LineIn::Read) {
  OSStatus status;

  auto instance = LineIn::Unwrap<LineIn>(info.Holder());

  status = AudioQueueStart(instance->audioQueue, nullptr);
  if (status != 0) exit(2);
}

NAN_MODULE_INIT(Initialize) {
  LineIn::Init(target);
}

NODE_MODULE(line_in, Initialize)
