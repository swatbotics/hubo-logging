#include "LogWriter.h"
#include <stdexcept>

LogWriter::LogWriter(): 
  _state(Closed),
  _file(0) {}

LogWriter::LogWriter(const std::string& filename, 
		     float frequency,
		     size_t bufsz):
  _state(Closed),
  _file(0)

{
  open(filename, frequency, bufsz);
}

LogWriter::LogWriter(FILE* fp, float frequency):
  _state(Closed),
  _file(0)
{
  open(fp, frequency);
}

LogWriter::~LogWriter() { reset(); }

void LogWriter::open(const std::string& filename, 
		     float frequency, 
		     size_t bufsz) {

  if (_state != Closed) {
    throw std::runtime_error("cannot open writer, already open!");
  }
  
  FILE* fp = fopen(filename.c_str(), "wb");
  if (!fp) {
    throw std::runtime_error("error opening " + filename + " for output");
  }

  if (bufsz == 0) {
    setvbuf(fp, NULL, _IONBF, 0);
  } else if (bufsz != DEFAULT_BUFFER_SIZE) {
    _blockbuffer.resize(bufsz);
    setvbuf(fp, &(_blockbuffer[0]), _IOFBF, bufsz);
  }

  open(fp, frequency);

}

void LogWriter::open(FILE* fp, float frequency) {

  if (_state != Closed) {
    throw std::runtime_error("cannot open writer, already open!");
  }

  _state = Opened;
  _file = fp;

  fprintf(fp, "%-10d %-10d %-10d %f\n", 0, 0, 0, frequency);

}

void LogWriter::finish() {

  if (_state == Opened) {
    writeHeader();
  }

  if (_state == Writing) {

    int nchan = _channels.size();
    int nsamp = _nsamples;
    int total = nchan * nsamp;

    
    fseek(_file, 0, SEEK_SET);
    fprintf(_file, "%-10d %-10d %-10d", total, nchan, nsamp);

    fclose(_file);
    _file = 0;
    _state = Closed;

  }

}

void LogWriter::reset() {

  finish();

  while (!_channels.empty()) {
    delete _channels.back().wrapper;
    _channels.pop_back();
  }

}

void LogWriter::addWrapper(FloatWrapper* wrapper,
			   const std::string& name,
			   const std::string& units) {

  if (_state == Writing) {
    throw std::runtime_error("cannot add channels while writer is writing!");
  }

  ChannelInfo info;
  info.name = name;
  info.units = units;
  info.wrapper = wrapper;

  _channels.push_back(info);

}

void LogWriter::writeHeader() {

  if (_state != Opened) {
    throw std::runtime_error("cannot write header while writer is not opened!");
  }
  
  if (_channels.empty()) {
    throw std::runtime_error("cannot write header for 0 channels\n");
  }    

  for (size_t i=0; i<_channels.size(); ++i) {
    fprintf(_file, "%s %s\n", _channels[i].name.c_str(), _channels[i].units.c_str());
  }
  fprintf(_file, "\n\n");

  _state = Writing;
  _nsamples = 0;

}

void LogWriter::writeSample() {

  if (_state == Opened) {
    writeHeader();
  }

  _rowbuffer.resize(_channels.size() * 4);
  char* out = &(_rowbuffer[0]);

  for (size_t i=0; i<_channels.size(); ++i) {
    float f = _channels[i].wrapper->getValue();
    const char* c = (const char*)&f;
    for (int j=0; j<4; ++j) {
      *out++ = c[3-j];
    }
  }

  size_t wrote = fwrite(&(_rowbuffer[0]), _rowbuffer.size(), 1, _file);

  if (wrote != 1) {
    throw std::runtime_error("error writing to file!");
  }

  ++_nsamples;

}
