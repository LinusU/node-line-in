#include <pulse/simple.h>
#include <pulse/error.h>

#include <node.h>
#include <nan.h>

static const size_t kBufferByteSize = 8192;

class LineIn : public Nan::ObjectWrap {
public:
  LineIn();
  ~LineIn();

  static void _emitBuffer(uv_work_t *work, int status);
  static NAN_MODULE_INIT(Init);

private:
  static NAN_METHOD(New);
  static NAN_METHOD(Read);

  void emitBuffer(void *data);
  void scheduleRead();

  bool reading;
  uv_work_t work;

  pa_sample_spec spec;
  pa_simple *pulseHandle;
};

struct WorkInfo {
  pa_simple *pulseHandle;
  void *audioBuffer;
  LineIn *instance;
};

void callback(uv_work_t *work) {
  auto workInfo = static_cast<WorkInfo*>(work->data);

  workInfo->audioBuffer = malloc(kBufferByteSize);

  int error;
  auto status = pa_simple_read(workInfo->pulseHandle, workInfo->audioBuffer, kBufferByteSize, &error);

  if (status < 0) exit(2);
}

LineIn::LineIn() {
  reading = false;
}

LineIn::~LineIn() {
  pa_simple_free(pulseHandle);
}

void LineIn::_emitBuffer(uv_work_t *work, int status) {
  auto workInfo = static_cast<WorkInfo*>(work->data);
  workInfo->instance->emitBuffer(workInfo->audioBuffer);
  delete workInfo;
}

void LineIn::emitBuffer(void *audioBuffer) {
  Nan::HandleScope scope;

  // Get a handle to the `push` function in JS land
  auto push = Nan::Get(handle(), Nan::New("push").ToLocalChecked()).ToLocalChecked().As<v8::Function>();

  // Create a Node.js buffer
  auto buffer = Nan::NewBuffer(static_cast<char*>(audioBuffer), kBufferByteSize).ToLocalChecked();

  // Push the copied buffer to JS land
  Nan::TryCatch tc;
  v8::Local<v8::Value> argv[] = { buffer };
  auto readMore = push->Call(Nan::GetCurrentContext()->Global(), 1, argv);

  // Propagate errors from `push`
  if (tc.HasCaught()) {
    // TODO: emit error event
    Nan::FatalException(tc);
  }

  reading = false;

  if (readMore->IsTrue()) {
    scheduleRead();
  }
}

void LineIn::scheduleRead() {
  if (reading) return;

  reading = true;

  auto workInfo = new WorkInfo;

  workInfo->instance = this;
  workInfo->pulseHandle = pulseHandle;

  work.data = static_cast<void*>(workInfo);

  uv_queue_work(uv_default_loop(), &work, callback, _emitBuffer);
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
  if (!info.IsConstructCall()) {
    return Nan::ThrowTypeError("Cannot call a class constructor");
  }

  auto instance = new LineIn();
  instance->Wrap(info.This());

  instance->spec.format = PA_SAMPLE_S16LE;
  instance->spec.channels = 2;
  instance->spec.rate = 44100;

  int error;
  instance->pulseHandle = pa_simple_new(nullptr, "Node.js", PA_STREAM_RECORD, nullptr, "Line In", &instance->spec, nullptr, nullptr, &error);
  if (instance->pulseHandle == nullptr) {
    Nan::ThrowError(pa_strerror(error));
  }
}

NAN_METHOD(LineIn::Read) {
  LineIn::Unwrap<LineIn>(info.Holder())->scheduleRead();
}

NAN_MODULE_INIT(Initialize) {
  LineIn::Init(target);
}

NODE_MODULE(line_in, Initialize)
