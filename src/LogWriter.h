#ifndef _LOGWRITER_H_
#define _LOGWRITER_H_

#include <string>
#include <vector>
#include <stdio.h>

class FloatWrapper {
public:
  virtual ~FloatWrapper() {}
  virtual float getValue() const =0;
};

template <class Tval> class TWrapper: public FloatWrapper {
public:
  const Tval* ptr;
  TWrapper(const Tval* p): ptr(p) {}
  virtual ~TWrapper() {}
  virtual float getValue() const { return float(*ptr); }
};

template <> class TWrapper<bool>: public FloatWrapper {
public:
  const bool* ptr;
  TWrapper(const bool* p): ptr(p) {}
  virtual ~TWrapper() {}
  virtual float getValue() const { return *ptr ? 1 : 0; }
};

class LogWriter {
public:

  enum {
    DEFAULT_BUFFER_SIZE = size_t(-1)
  };

  LogWriter();
  LogWriter(const std::string& filename, 
	    float frequency, 
	    size_t bufsz=DEFAULT_BUFFER_SIZE);

  LogWriter(FILE* fp, float frequency);

  ~LogWriter();

  void open(FILE* fp, float frequency);

  void open(const std::string& filename, 
	    float frequency,
	    size_t bufsz=DEFAULT_BUFFER_SIZE);

  void finish();
  void reset();

  void addWrapper(FloatWrapper* wrapper,
		  const std::string& name, 
		  const std::string& units="1");

  template <class Tval> 
  void add(const Tval* ptr, 
	   const std::string& name,
	   const std::string& units="1") {

    TWrapper<Tval>* wrapper = new TWrapper<Tval>(ptr);
    addWrapper(wrapper, name, units);

  }

  void sortChannels();

  void writeHeader();
  void writeSample();
  
private:

  enum State {
    Closed,
    Opened,
    Writing
  };


  struct ChannelInfo {
    std::string name;
    std::string units;
    FloatWrapper* wrapper;
    bool operator<(const ChannelInfo& b) const;
  };
  
  typedef std::vector<ChannelInfo> ChannelInfoArray;

  State _state;
  
  FILE* _file;
  float _frequency;

  ChannelInfoArray _channels;
  size_t _nsamples;

  std::vector<char> _rowbuffer;
  std::vector<char> _blockbuffer;

};

#endif
