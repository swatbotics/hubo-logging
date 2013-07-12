#ifndef _PLOTMAINWINDOW_H_
#define _PLOTMAINWINDOW_H_

#include <QMainWindow>
#include "LogReader.h"

#include <vector>

class MzDataSet;
class MzPlotData;
class MzPlotCurve;
class MzPlot;
class MzDataSetListView;
typedef std::pair<MzPlot*, MzPlot*> MzPlotPair;

class QLineEdit;
class QPushButton;
class QGridLayout;
class QwtScaleWidget;

struct PlotVar {
  size_t fileIndex;
  std::string varName;
};

typedef std::list<PlotVar> PlotVars;
typedef std::list<PlotVars> PlotSet;

class PlotMainWindow: public QMainWindow {

  Q_OBJECT

public:

  enum {
    MAX_PLOTS = 12,
    MAX_ZOOM = 100,
    MIN_ZOOM = 10,
  };

  PlotMainWindow(QWidget* parent=0);
  virtual ~PlotMainWindow();

  bool shouldAddCurve(MzPlot* p);
  size_t numDatasets() const;                                

  static bool readPlotSet(std::istream& istr, PlotSet& set);
  static bool writePlotSet(std::ostream& ostr, const PlotSet& set);

  void getPlotSet(PlotSet& set) const;
  void mergePlotSet(const PlotSet& set, bool clear);

  size_t getFileIndex(const MzDataSet* dataset) const;
  const MzDataSet* getDataSet(size_t index) const;
  MzDataSet* getDataSet(size_t index);

  static QString defaultPlotSetFile();

public slots:

  bool savePlotSet(const QString& filename) const;
  bool mergePlotSet(const QString& filename, bool clear);
  void savePlotSet();
  void loadPlotSet();
  void mergePlotSet();

  void addPlot();
  void removePlot(MzPlot* p);
  void removeData(MzPlot* p, MzPlotData* d);
  void removeEmptyPlots();
  void clearPlot(MzPlot* p);

  void setMarker(double t);
  void setMarker(double t, bool broadcast);
  void setStartTime(double t0);
  void setEndTime(double t1);
  void setRange(double t0, double t1);
  void setRange(double t0, double t1, double m);
  void zoomIn();
  void zoomOut();
  void zoomReset();

  void clearSearch();
  void searchChanged();

  void openData();
  void openData(const QString& s);

protected:
  virtual void closeEvent(QCloseEvent *e);

private:

  void redoPlotLayout();
  void updatePlotScales();

  MzDataSetListView* _datalist;
  std::list< MzPlotPair > _plots;
  QWidget* _plotHolder;
  QGridLayout* _plotLayout;
  double _minTime, _maxTime;
  double _startTime, _endTime;
  double _markerTime;
  double _zoom; 
  QwtScaleWidget* _scl;

  QLineEdit* _searchEdit;

};

#endif
