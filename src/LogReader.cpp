#include "LogReader.h"
#include <stdio.h>
#include <stdexcept>

LogReader::LogReader(): 
  _frequency(0), _numTicks(0) {}

LogReader::LogReader(const std::string& filename):
  _frequency(0), _numTicks(0) {

  open(filename);

}

void LogReader::open(const std::string& filename) {

  _names.clear();
  _units.clear();
  _data.clear();

  FILE* fp = fopen(filename.c_str(), "rb");
  if (!fp) { 
    throw std::runtime_error("Error opening " + filename);
  }

  int total_n_numbers, n_channels, n_points;
  
  if (fscanf(fp, "%d%d%d%f", 
	     &total_n_numbers, 
	     &n_channels,
	     &n_points,
	     &_frequency) != 4) {
    throw std::runtime_error("error reading header");
  }

  if (total_n_numbers != n_channels * n_points) {
    throw std::runtime_error("failed sanity check n_channels * n_points");
  }

  _numTicks = n_points;

  char name[1024], unit[1024];

  for (int i=0; i<n_channels; ++i) {
    if (fscanf(fp, "%1023s%1023s", name, unit) != 2) {
      throw std::runtime_error("error reading name/units");
    }
    _names.push_back(name);
    _units.push_back(unit);
  }

  fscanf(fp, "%c%c%c", name, name, name);

  for (int i=0; i<total_n_numbers; ++i) {
    float f;
    char* c = (char*)&f;
    for (int j=0; j<4; ++j) {
      if (fread(c+3-j, 1, 1, fp) != 1) {
	throw std::runtime_error("error reading float data");
      }
    }
    _data.push_back(f);
  }
  
  fclose(fp);

}

float LogReader::frequency() const { 
  return _frequency;
}

size_t LogReader::numChannels() const {
  return _names.size();
}

size_t LogReader::numTicks() const {
  return _numTicks;
}

const std::string& LogReader::channelName(size_t i) const {
  return _names[i];
}

const std::string& LogReader::channelUnits(size_t i) const {
  return _units[i];
}

size_t LogReader::lookupChannel(const std::string& name) const {
  for (size_t i=0; i<_names.size(); ++i) {
    if (_names[i] == name) { return i; }
  }
  return npos;
}

float LogReader::operator()(size_t channel, size_t tick) const {
  return _data[tick * _names.size() + channel];
}

