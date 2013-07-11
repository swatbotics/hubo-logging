#ifndef _LOGREADER_H_
#define _LOGREADER_H_

#include <string>
#include <vector>

class LogReader {
public:

  enum { npos = size_t(-1) };


  LogReader();

  LogReader(const std::string& filename);
  
  void open(const std::string& filename);

  float frequency() const;
  size_t numChannels() const;
  size_t numTicks() const;
  
  const std::string& channelName(size_t i) const;
  const std::string& channelUnits(size_t i) const;

  size_t lookupChannel(const std::string& name) const;

  float operator()(size_t channel, size_t tick) const;

private:

  float _frequency;
  size_t _numTicks;

  std::vector<std::string> _names;
  std::vector<std::string> _units;

  std::vector<float> _data;

};

#endif
