#include "PlotMainWindow.h"
#include <vector>
#include <sstream>
#include <fstream>

#include <QLayout>
#include <QColor>
#include <QStatusBar>
#include <QLabel>
#include <QCursor>
#include <Q3ListView>
#include <QLineEdit>
#include <QSplitter>
#include <QPushButton>
#include <QFileDialog>
#include <QPainter>
#include <QBitmap>
#include <QPixmap>
#include <QDir>
#include <QAction>
#include <QMenuBar>
#include <QApplication>
#include <Q3VBox>
#include <Q3HBox>
#include <Q3PopupMenu>
#include <QWheelEvent>

#include <qwt_data.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>


#include <iostream>

// Right-click handler to smallscale or delete curve
// +/- to zoom in or out
// Left-click in plots to move line
// Need to open log, have stuff from logs
// Nice to save templates based on variable names

// dataset knows all its plotdata
// plotdata all its plots, curves
// plotdata knows its dataset
// curve knows its plot
// curve knows its dataset
// plot knows all its plotdatas, curves


class MzPlotData: public QwtData {
public:

  MzPlotData(const QString& f, const QString& n, MzDataSet* d, size_t size, const float* xdata, const float* ydata):
  filename(f), name(n), dataset(d), _size(size), _xdata(xdata), _ydata(ydata) {
  } 
  
  virtual QwtData* copy() const { 
    return new MzPlotData(filename, name, dataset, _size, _xdata, _ydata);
  }

  virtual size_t size() const {
    return _size;
  }

  virtual double x(size_t i) const {
    return _xdata[i];
  }

  virtual double y(size_t i) const {
    return _ydata[i];
  }

  void getBounds(double xmin, double xmax, double& ymin, double& ymax) {
    bool first = (ymin > ymax);
    for (size_t i=0; i<_size; ++i) {
      double xi = x(i);
      if (xi < xmin || xi > xmax) { continue; }
      double yi = y(i);
      if (first || yi < ymin) { ymin = yi; }
      if (first || yi > ymax) { ymax = yi; }
      first = false;
    }
  }

  const QString filename;
  const QString name;
  MzDataSet* const dataset;
  std::list<MzPlotCurve*> curves;

private:

  const size_t _size;
  const float* _xdata;
  const float* _ydata;
  
};


class MzPlotCurve: public QwtPlotCurve {
public:


  MzPlotCurve(MzPlotData* d, MzPlot* p):
  QwtPlotCurve(d->name), data(d), plot(p), cidx(0)
  {
    setData(*d);
    setPen(QPen(Qt::blue));
  }

  MzPlotData* data;
  MzPlot* plot;
  int cidx;
  
};


class MzDataSet {
public:

  MzDataSet(const QString& name, LogReader* r):
  _dataset(r) { 

    _name = QFileInfo(name).baseName();
    
    _tdata = 0;
    _owntdata = 0;

    int m = _dataset->numTicks();
    int n = _dataset->numChannels();

    int robottimestamp = -1;
    int timevar = -1;

    for (int j=0; j<n; ++j) {
      QString name = _dataset->channelName(j).c_str();
      if (robottimestamp == -1 && name == "robot.timestamp") {
        robottimestamp = j;
      } else if (timevar == -1 && name.find("time") >= 0) {
        timevar = j;
      }
    }

    if (robottimestamp != -1) {
      timevar = robottimestamp;
    }

    if (timevar != -1) {
      std::cerr << "using " << _dataset->channelName(timevar) << " as time reference!\n";
      _tdata = _dataset->channelData(timevar);
    }

    if (!_tdata) {
      _owntdata = new float[m];
      for (int i=0; i<m; ++i) {
        _owntdata[i] = i/_dataset->frequency();
      }
      _tdata = _owntdata;
    }

    std::cerr << "min time = " << minTime() << "\n";
    std::cerr << "max time = " << maxTime() << "\n";
    std::cerr << "expected max time = " << minTime() + (m-1)/_dataset->frequency() << "\n";

    for (int j=0; j<n; ++j) {
      MzPlotData* d = new MzPlotData(_name,
                                     _dataset->channelName(j).c_str(),
                                     this, m, _tdata, _dataset->channelData(j));
      _data.push_back(d);
    }

  }
  
  ~MzDataSet() { 

    delete _dataset; 
    _dataset = 0;

    if (_owntdata) {
      delete[] _owntdata;
      _owntdata = 0;
    }

  }

  const QString& name() const {
    return _name;
  }

  const std::vector<MzPlotData*>& data() const {
    return _data;
  }

  float minTime() const {
    return _tdata[0];
  }

  float maxTime() const {
    return _tdata[_dataset->numTicks()-1];
  }

  MzPlotData* lookupVariable(const QString& name) {
    for (size_t i=0; i<_data.size(); ++i) {
      if (_data[i]->name == name) { return _data[i]; }
    }
    return 0;
  }

  const MzPlotData* lookupVariable(const QString& name) const {
    for (size_t i=0; i<_data.size(); ++i) {
      if (_data[i]->name == name) { return _data[i]; }
    }
    return 0;
  }

private:

  QString _name;
  LogReader* _dataset;
  const float* _tdata;
  float* _owntdata;

  std::vector<MzPlotData*> _data;

};


class MzPlotDataItem: public Q3ListViewItem {
public:

  MzPlotDataItem(MzPlotData* d,
                 Q3ListViewItem* parent):
    Q3ListViewItem(parent, d->name), data(d) {}
  
  MzPlotDataItem(MzPlotData* d,
                 Q3ListViewItem* parent,
                 Q3ListViewItem* after):
    Q3ListViewItem(parent, after, d->name), data(d) {}

  MzPlotData* data;

};


class MzDataSetItem: public Q3ListViewItem {
public:

  MzDataSetItem(MzDataSet* set,
                Q3ListView* parent,
                Q3ListViewItem* after):
    Q3ListViewItem(parent, after, set->name()), dataset(set) 
  {
    Q3ListViewItem* prev = 0;
    for (size_t i=0; i<set->data().size(); ++i) {
      if (!prev) {
        prev = new MzPlotDataItem(set->data()[i], this);
      } else {
        prev = new MzPlotDataItem(set->data()[i], this, prev);
      }
    }
    setOpen(true);
  }
  
  virtual ~MzDataSetItem() {}

  void filter(const QString& s) {
    Q3ListViewItem* c = firstChild();
    for ( ; c; c = c->nextSibling()) {
      c->setVisible(s.isNull() || s.isEmpty() || (c->text(0).find(s) >= 0));
    }
  }
    
  MzDataSet* dataset;
                
};


class MzDataSetListView: public Q3ListView {
public:

  MzDataSetListView(PlotMainWindow* w, QWidget* parent):
  Q3ListView(parent) {

    addColumn("Variable");
    setRootIsDecorated(true);
    setSelectionMode(Single);
    setSortColumn(-1);

    QFont f = font();
    f.setPointSize(10);
    setFont(f);


  }

  virtual ~MzDataSetListView() { }

  void addDataSet(MzDataSet* set) {
    datasets.push_back(new MzDataSetItem(set, this, lastItem()));
  }

  void filter(const QString& str) {
    for (std::list<MzDataSetItem*>::const_iterator i=datasets.begin();
         i!=datasets.end(); ++i) {
      MzDataSetItem* d = *i;
      d->filter(str);
    }
  }

  std::list<MzDataSetItem*> datasets;

};


class MzPlotPicker: public QwtPlotPicker {
public:

  MzPlotPicker(QwtPlotCanvas* pc): QwtPlotPicker(pc) {
    //setSelectionMode(ClickSelection);
    setSelectionFlags(PointSelection | DragSelection);
    //setTrackerMode(AlwaysOn);
  }
  
  virtual ~MzPlotPicker() {}

protected:

  virtual bool accept(QwtPolygon& pa) {
    if (pa.isEmpty()) { 
      std::cerr << "accepting empty\n";
    } else {
      QPoint p = pa[0];
      std::cerr << "accepting " << p.x() << ", " << p.y() << "\n";
    }
    return true;
  }

  virtual void begin() {
    std::cerr << "got begin!\n";
    QwtPlotPicker::begin();
  }

  virtual bool end(bool ok) {
    std::cerr << "got end, ok=" << ok << "\n";
    const QwtPolygon& pa = selection();
    if (pa.isEmpty()) { 
      std::cerr << "sel is empty\n";
      return true;
    } else {
      for (int i=0; i<pa.count(); ++i) {
        QPoint p = pa[i];
        std::cerr << "sel has point " << p.x() << ", " << p.y() << "\n";
      }
    }
    return !(pa.isEmpty());
  }


  
};


enum { NUM_COLORS = 8 };

static const QColor colors[NUM_COLORS] = {
  Qt::blue,
  Qt::red,
  Qt::darkGreen,
  Qt::magenta,
  Qt::darkCyan,
  Qt::darkYellow,
  Qt::darkGreen,
  Qt::darkGray,
};

static QPixmap makePixmap(const QColor& color) {
  QPixmap m(12, 12);
  m.fill(Qt::white);
  QPainter p(&m);
  p.setPen(QPen(color, 2));
  p.drawLine(0,6,11,6);
  p.end();
  m.setMask(m.createHeuristicMask());
  return m;
}


class MzPlot: public QwtPlot {
public:

  void getBounds(double xmin, double xmax,
                 double& ymin, double& ymax) {

    for (std::list<MzPlotCurve*>::const_iterator i=curves.begin(); 
         i!=curves.end(); ++i) {
      MzPlotData* d = (*i)->data;
      d->getBounds(xmin, xmax, ymin, ymax);
    }

  }

  void doScale(double xmin, double xmax) {
    setAxisScale(xBottom, xmin, xmax);
    double ymin = 100;
    double ymax = 0;
    getBounds(xmin, xmax, ymin, ymax);
    if (ymax < ymin) {
      setAxisAutoScale(yLeft);
    } else {
      int mm = axisMaxMajor(yLeft);
      double ystep = (mm ? (ymax-ymin)/mm : 0);
      QwtScaleEngine* e = axisScaleEngine(yLeft);
      e->autoScale(mm, ymin, ymax, ystep);
      setAxisScale(yLeft, ymin, ymax, ystep);
    }
  }
  
  MzPlot(PlotMainWindow* main, QWidget* parent): QwtPlot(parent), _main(main) {

    setCanvasBackground(Qt::white);

    QwtPlotGrid* g = new QwtPlotGrid();
    g->setMajPen(QColor(230,230,230));
    g->attach(this);

    setAxisMaxMinor(xBottom, 0);
    setAxisMaxMinor(yLeft, 0);
    
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    marker = new QwtPlotMarker();
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setLinePen(QPen(Qt::red));
    marker->setXValue(0);
    marker->attach(this);
    
    _allowDragging = true;

    //picker = new MzPlotPicker(canvas());

  }

  virtual ~MzPlot() { 
    while (!curves.empty()) {
      MzPlotCurve* c = curves.front();
      c->data->curves.remove(c);
      curves.erase(curves.begin());
      c->detach();
      delete c;
    }
  }

  void setDraggingAllowed(bool b) {
    _allowDragging = b;
  }
  
  virtual QSize sizeHint() const {
    QSize sz = QwtPlot::sizeHint();
    sz.setHeight(100);
    return sz;
  }
    
  virtual QSize minimumSizeHint() const {
    QSize sz = QwtPlot::minimumSizeHint();
    sz.setHeight(100);
    return sz;
  }

  void removeData(MzPlotData* d) {
    for (std::list<MzPlotCurve*>::iterator i=curves.begin(); 
         i!=curves.end(); ++i) {
      MzPlotCurve* c = *i;
      if (c->data == d) {
        d->curves.remove(c);
        curves.erase(i);
        c->detach();
        delete c;
        return;
      }
    }

  }

  bool addData(MzPlotData* d) {
    

    int ccount[NUM_COLORS] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    for (std::list<MzPlotCurve*>::const_iterator i=curves.begin(); 
         i!=curves.end(); ++i) {
      MzPlotCurve* c = *i;
      if (c->data == d) { return false; }
      ++ccount[c->cidx];
    }

    int min = ccount[0];
    int minidx = 0;
    for (int i=1; i<NUM_COLORS; ++i) {
      if (ccount[i] < min) { min = ccount[i]; minidx = i; }
    }

    MzPlotCurve* c = new MzPlotCurve(d, this);
    c->setPen(colors[minidx]);
    c->cidx = minidx;
    c->attach(this);
    curves.push_back(c);
    d->curves.push_back(c);
    setAxisAutoScale(yLeft);
    replot();
    return true;
  }

  std::list<MzPlotCurve*> curves;
  QwtPlotMarker* marker;
  //MzPlotPicker* picker;

protected:

  QPoint translateToCanvas(const QPoint& p) {
    return canvas()->mapFromGlobal(mapToGlobal(p));
  }

  QwtDoublePoint myInv(const QPoint& p) {
    QPoint pos = translateToCanvas(p);
    double px = invTransform(xBottom, pos.x());
    double py = invTransform(yLeft, pos.y());
    return QwtDoublePoint(px, py);
  }

  virtual void wheelEvent(QWheelEvent* e) {
    int absdelta = abs(e->delta()) / 120;
    bool in = e->delta() > 0;
    //std::cerr << "absdelta = " << absdelta << "\n";
    //return;
    for (int i=0; i<absdelta; ++i) {
      in ? _main->zoomIn() : _main->zoomOut();
    }
  }
  
  virtual void mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) { return; }
    if (_main->shouldAddCurve(this)) {
      _dragging = false;
    } else {
      _mpos = e->pos();
      QwtDoublePoint p = myInv(e->pos());
      _main->setMarker(p.x());
      _dragging = _allowDragging;
    }
  }

  virtual void mouseMoveEvent(QMouseEvent* e) {
    if (_dragging) {
      if (e->pos() != _mpos) {
        _mpos = e->pos();
        QwtDoublePoint p = myInv(_mpos);
        _main->setMarker(p.x());
      }
    }
  }

  virtual void mouseReleaseEvent(QMouseEvent* e) {
    if (_dragging) {
      if (e->pos() != _mpos) {
        _mpos = e->pos();
        QwtDoublePoint p = myInv(_mpos);
        _main->setMarker(p.x());
      }
    }
    _dragging = false;
  }

  virtual void contextMenuEvent(QContextMenuEvent* e) {

    Q3PopupMenu* m = new Q3PopupMenu(this);

    int clearid = 0;

    if (!curves.empty()) {

      int cnt = 0;

      QString fname = curves.front()->data->filename;
      bool haveMany = _main->numDatasets() > 1;

      for (std::list<MzPlotCurve*>::const_iterator i=curves.begin(); i!=curves.end(); ++i) {
        MzPlotCurve* c = *i;
        MzPlotData* d = c->data;
        QString cname;
        if (haveMany) {
          cname = d->filename + ": " + d->name;
        } else {
          cname = d->name;
        }
        m->insertItem(makePixmap(colors[c->cidx%NUM_COLORS]), "Remove " + cname, cnt++, -1);
      }

      m->insertSeparator();

      clearid = m->insertItem("&Clear plot", curves.size());

    }

    int deleteid = m->insertItem("&Delete plot", curves.size()+1);
    int rval = m->exec(QCursor::pos());

    delete m;

    if (rval == clearid) {
      _main->clearPlot(this);
    } else if (rval == deleteid) {
      _main->removePlot(this);
    } else {
      int cnt = 0;
      for (std::list<MzPlotCurve*>::iterator i=curves.begin(); i!=curves.end(); ++i) {
        if (cnt++ == rval) {
          _main->removeData(this, (*i)->data);
          break;
        }
      }
    }

  }

  QPoint _mpos;
  PlotMainWindow* _main;
  bool _dragging;
  bool _allowDragging;

};

static QAction* makeAction(const QString& label,
			   const QKeySequence& sequence,
			   QObject* parent) {

  QAction* a = new QAction(label, parent);
  a->setShortcut(sequence);
  return a;

}

PlotMainWindow::PlotMainWindow(QWidget* parent): QMainWindow(parent) {

  QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(splitter);

  _plotHolder = new QWidget(splitter);
  _plotLayout = 0;

  
  Q3VBox* vb = new Q3VBox(splitter);

  splitter->setResizeMode(_plotHolder, QSplitter::Stretch);
  splitter->setResizeMode(vb, QSplitter::KeepSize);

  vb->setSpacing(6);
  vb->setMargin(11);
  
  Q3HBox* sb = new Q3HBox(vb);
  //sb->setSpacing(6);
  _searchEdit = new QLineEdit(sb);
  QPushButton* cb = new QPushButton("X", sb);
  connect(cb, SIGNAL(clicked()), this, SLOT(clearSearch()));

  _datalist = new MzDataSetListView(this, vb);
  _datalist->setMinimumWidth(250);
  _datalist->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

  _searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  cb->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  connect(_searchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(searchChanged()));

  _plotHolder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

  _minTime = _startTime = 0;
  _maxTime = _endTime = 10;
  _markerTime = 5;
  _zoom = 1;

  for (int i=0; i<3; ++i) { addPlot(); }

  setRange(_startTime, _endTime, _markerTime);

  statusBar()->message("This is a test");

  QAction* fileOpenAction = makeAction("&Open", QKeySequence("CTRL+O"), this);
  connect(fileOpenAction, SIGNAL(activated()), this, SLOT(openData()));

  QAction* fileOpenSetAction = makeAction("Open &plot set", QKeySequence("CTRL+SHIFT+O"), this);
  connect(fileOpenSetAction, SIGNAL(activated()), this, SLOT(loadPlotSet()));

  QAction* fileMergeSetAction = makeAction("&Merge plot set", QKeySequence("CTRL+M"), this);
  connect(fileMergeSetAction, SIGNAL(activated()), this, SLOT(mergePlotSet()));

  QAction* fileSaveSetAction = makeAction("&Save plot set", QKeySequence("CTRL+S"), this);
  connect(fileSaveSetAction, SIGNAL(activated()), this, SLOT(savePlotSet()));

  QAction* fileNewAction = makeAction("&New plot", QKeySequence("CTRL+N"), this);
  connect(fileNewAction, SIGNAL(activated()), this, SLOT(addPlot()));

  QAction* fileQuitAction = makeAction("&Quit", QKeySequence("CTRL+Q"), this);
  connect(fileQuitAction, SIGNAL(activated()), this, SLOT(close()));

  QAction* viewZoomInAction = makeAction("Zoom &In", QKeySequence("CTRL+="), this);
  connect(viewZoomInAction, SIGNAL(activated()), this, SLOT(zoomIn()));

  QAction* viewZoomOutAction = makeAction("Zoom &Out", QKeySequence("CTRL+-"), this);
  connect(viewZoomOutAction, SIGNAL(activated()), this, SLOT(zoomOut()));

  QAction* viewZoomResetAction = makeAction("Zoom &1s", QKeySequence("CTRL+0"), this);
  connect(viewZoomResetAction, SIGNAL(activated()), this, SLOT(zoomReset()));

  QMenu* f = new QMenu("&File", this);
  fileOpenAction->addTo(f);
  f->insertSeparator();
  fileOpenSetAction->addTo(f);
  fileMergeSetAction->addTo(f);
  fileSaveSetAction->addTo(f);
  f->insertSeparator();
  fileNewAction->addTo(f);
  f->insertSeparator();
  fileQuitAction->addTo(f);

  QMenu* v = new QMenu("&View", this);
  viewZoomInAction->addTo(v);
  viewZoomOutAction->addTo(v);
  v->insertSeparator();
  viewZoomResetAction->addTo(v);

  QMenuBar* mb = menuBar();
  mb->addMenu(f);
  mb->addMenu(v);


}

PlotMainWindow::~PlotMainWindow() { 

}


void PlotMainWindow::removePlot(MzPlot* p) {
  
  clearPlot(p);

  for (std::list< MzPlotPair >::iterator i=_plots.begin();
       i != _plots.end(); ++i ) {

    if (p == i->first || p == i->second) {
      delete i->first;
      delete i->second;
      _plots.erase(i);
      redoPlotLayout();
      return;
    }

  }


}

void PlotMainWindow::clearPlot(MzPlot* p) {

  while (!p->curves.empty()) {
    removeData(p, p->curves.front()->data);
  }


}

void PlotMainWindow::removeData(MzPlot* p, MzPlotData* d) {

  for (std::list< MzPlotPair >::const_iterator i=_plots.begin();
       i != _plots.end(); ++i ) {

    if (p == i->first || p == i->second) {
      i->first->removeData(d);
      i->second->removeData(d);
      updatePlotScales();
      return;
    }
  }


}



void PlotMainWindow::setMarker(double t, bool broadcast) {
  statusBar()->message(QString("Marker set to %1").arg(t, 0, 'f', 5));
  setRange(_startTime, _endTime, t);
}

void PlotMainWindow::setMarker(double t) {
  setMarker(t, true);
}

void PlotMainWindow::setStartTime(double t0) {
  setRange(t0, _endTime, _markerTime);
}

void PlotMainWindow::setEndTime(double t1) {
  setRange(_startTime, t1, _markerTime);
}

void PlotMainWindow::setRange(double t0, double t1) {
  setRange(t0, t1, _markerTime);
}

void PlotMainWindow::zoomIn() { 
  _zoom *= 0.75;
  if (_zoom < 1.0/MAX_ZOOM) { _zoom = 1.0/MAX_ZOOM; }
  updatePlotScales();
}

void PlotMainWindow::zoomOut() { 
  _zoom *= 1.5;
  if (_zoom > MIN_ZOOM) { _zoom = MIN_ZOOM; }
  updatePlotScales();
}

void PlotMainWindow::zoomReset() {
  _zoom = 1;
  updatePlotScales();
}

static void checkRange(double& min, double& max) {
  if (min > max) { std::swap(min, max); }
}

static double clamp(double v, double min, double max) {
  if (v < min) { v = min; }
  if (v > max) { v = max; }
  return v;
}


void PlotMainWindow::setRange(double t0, double t1, double tm) {

  _startTime = clamp(t0, _minTime, _maxTime);
  _endTime = clamp(t1, _minTime, _maxTime);
  checkRange(_startTime, _endTime);

  _markerTime = clamp(tm, _startTime, _endTime);
  
  updatePlotScales();
  
}


void PlotMainWindow::clearSearch() {
  _searchEdit->clear();
  searchChanged();
}

void PlotMainWindow::searchChanged() {
  _datalist->filter(_searchEdit->text());
}


bool PlotMainWindow::shouldAddCurve(MzPlot* p) {

  Q3ListViewItem* i = _datalist->selectedItem();
  if (!i) { return false; }
  
  MzPlotDataItem* d = dynamic_cast<MzPlotDataItem*>(i);
  if (!d) { return false; }
  
  for (std::list< MzPlotPair >::const_iterator i=_plots.begin();
       i != _plots.end(); ++i ) {

    if (p == i->first || p == i->second) {
      if (!i->first->addData(d->data)) {
        return false;
      }
      i->second->addData(d->data);
      _datalist->clearSelection();
      updatePlotScales();
      std::cerr << "added data " << d->data->name.ascii() << " to plot.\n";
      return true;
    }

  }

  return false;

}

void PlotMainWindow::addPlot() {

  if (_plots.size() >= MAX_PLOTS) { return; }

  std::cerr << "adding a new plot...\n";

  MzPlot* p1 = new MzPlot(this, _plotHolder);
  MzPlot* p2 = new MzPlot(this, _plotHolder);

  p1->show();
  p2->show();

  p1->setMinimumWidth(300);
  p2->setFixedWidth(250);
  p2->setDraggingAllowed(false);
  p2->setAxisMaxMajor(QwtPlot::xBottom, 4);

  MzPlotPair p(p1, p2);
  _plots.push_back(p);

  redoPlotLayout();
  updatePlotScales();


}

void PlotMainWindow::openData() {

  QString s = 
    QFileDialog::getOpenFileName(QDir::currentDirPath(),
                                 "Log files (*.data *.log);; Any files (*)");

  if (s.isNull() || s.isEmpty()) { return; }

  openData(s);

}

void PlotMainWindow::openData(const QString& s) {


  LogReader* r = 0;

  try {
    r = new LogReader(s.ascii());
  } catch (...) {
  }

  if (!r) { return; }

  MzDataSet* d = new MzDataSet(s, r);
  _datalist->addDataSet(d);
  
  _minTime = 0;
  _maxTime = 100;

  bool first = true;
  for (std::list<MzDataSetItem*>::const_iterator i=_datalist->datasets.begin();
       i!=_datalist->datasets.end(); ++i) {
    MzDataSet* d = (*i)->dataset;
    if (first || d->minTime() < _minTime) { _minTime = d->minTime(); }
    if (first || d->maxTime() > _maxTime) { _maxTime = d->maxTime(); }
    first = false;
  }

  checkRange(_minTime, _maxTime);

  _startTime = _minTime;
  _endTime = _maxTime;

  setRange(_startTime, _endTime, _markerTime);

  
  if (!_searchEdit->text().isEmpty()) {
    searchChanged();
  }

  
}

void PlotMainWindow::updatePlotScales() {

  for (std::list< MzPlotPair >::const_iterator i=_plots.begin();
       i != _plots.end(); ++i ) {
    
    i->first->doScale(_startTime, _endTime);
    i->first->marker->setXValue(_markerTime);

    i->second->doScale(_markerTime-_zoom, _markerTime+_zoom);
    i->second->marker->setXValue(_markerTime);

    i->first->replot();
    i->second->replot();

  }
  
}

void PlotMainWindow::redoPlotLayout() {
  
  MzPlotPair p(0,0);
  int row = 0;

  delete _plotLayout;
  _plotLayout = new QGridLayout(_plotHolder);
  _plotLayout->setMargin(11);
  _plotLayout->setSpacing(8);

  for (std::list< MzPlotPair >::const_iterator i=_plots.begin();
       i != _plots.end(); ++i ) {

    p = *i;
    _plotLayout->addWidget(p.first, row, 0);
    _plotLayout->addWidget(p.second, row++, 1);
    p.first->enableAxis(QwtPlot::xBottom, false); 
    p.second->enableAxis(QwtPlot::xBottom, false); 

  }

  if (p.first) {
    p.first->enableAxis(QwtPlot::xBottom, true);
    p.second->enableAxis(QwtPlot::xBottom, true);
  }

  
}

size_t PlotMainWindow::numDatasets() const {
  //return _datalist->childCount();
  return 0;
}


bool PlotMainWindow::readPlotSet(std::istream& istr, PlotSet& set) {
  set.clear();
  if (!istr) { return 0; }
  std::string buf;
  PlotVars pvars;
  int line = 0;
  while (1) {
    if (!std::getline(istr, buf)) { break; }
    ++line;
    if (buf.empty()) {
      if (!pvars.empty()) { set.push_back(pvars); pvars.clear(); }
    } else {
      std::istringstream sstr(buf);
      PlotVar pvar;
      if (!(sstr >> pvar.fileIndex)) {
        std::cerr << "syntax error on line " << line << ": couldn't parse file index\n";
        return false;
      }
      int delim = sstr.get();
      if (delim != ':') {
        std::cerr << "syntax error on line " << line << ": expected ':'\n";
        return false;
      }
      if (!(sstr >> pvar.varName)) {
        std::cerr << "could not parse varname\n";
      }
      pvars.push_back(pvar);
    }
  }
  if (!pvars.empty()) { set.push_back(pvars); }
  return !(set.empty());
}

bool PlotMainWindow::writePlotSet(std::ostream& ostr, const PlotSet& set) {
  if (!ostr) { return false; }
  for (PlotSet::const_iterator i=set.begin(); i!=set.end(); ++i) {
    if (i != set.begin()) { ostr << "\n"; }
    const PlotVars& pvars = *i;
    for (PlotVars::const_iterator j=pvars.begin(); j!=pvars.end(); ++j) {
      const PlotVar& pvar = *j;
      ostr << pvar.fileIndex << ":" << pvar.varName << "\n";
    }
  }
  return ostr;
}

void PlotMainWindow::getPlotSet(PlotSet& set) const {

  set.clear();
  // for each plot
  for (std::list<MzPlotPair>::const_iterator i=_plots.begin(); i!=_plots.end(); ++i) {
    const MzPlot* plot = i->first;
    // for each curve
    const std::list<MzPlotCurve*>& curves = plot->curves;
    PlotVars pvars;
    for (std::list<MzPlotCurve*>::const_iterator j=curves.begin(); j!=curves.end(); ++j) {
      const MzPlotCurve* curve = *j;
      const MzPlotData* data = curve->data;
      const MzDataSet* dataset = data->dataset;
      const QString& name = data->name;
      PlotVar pvar;
      pvar.fileIndex = getFileIndex(dataset);
      pvar.varName = name.ascii();
      pvars.push_back(pvar);
    }
    if (!pvars.empty()) { set.push_back(pvars); }
  }

}

void PlotMainWindow::removeEmptyPlots() {

  std::list<MzPlotPair>::iterator i = _plots.begin();
  while (i != _plots.end()) {
    std::list<MzPlotPair>::iterator next = i; ++next;
    MzPlot* plot = i->first;
    if (plot->curves.empty()) {
      if (_plots.size() == 1) { return; }
      removePlot(plot);
    }
    i = next;
  }

}

void PlotMainWindow::mergePlotSet(const PlotSet& set, bool clear) {


  if (clear) {

    // delete all plots but one
    while (_plots.size() > 1) { 
      removePlot(_plots.back().second);
    }
  
    // clear the current plot
    clearPlot(_plots.back().second);
  
  } else {

    removeEmptyPlots();
    
  }

  for (PlotSet::const_iterator i=set.begin(); i!=set.end(); ++i) {
    const PlotVars& pvars = *i;
    if (!_plots.back().second->curves.empty()) {
      addPlot();
    }
    for (PlotVars::const_iterator j=pvars.begin(); j!=pvars.end(); ++j) {
      MzDataSet* dataset = getDataSet(j->fileIndex);
      if (!dataset) { continue; }
      MzPlotData* data = dataset->lookupVariable(j->varName.c_str());
      if (!data) { continue; }
      _plots.back().first->addData(data);
      _plots.back().second->addData(data);
    }
  }
  
}

bool PlotMainWindow::savePlotSet(const QString& filename) const {

  PlotSet set;
  getPlotSet(set);

  std::ofstream ostr(filename.ascii());
  if (!ostr) { return false; }
  if (!writePlotSet(ostr, set)) { return false; }

  return true;

}

bool PlotMainWindow::mergePlotSet(const QString& filename, bool clear) {

  PlotSet set;
  
  std::ifstream istr(filename.ascii());
  if (!istr) { 
    std::cerr << "error opening file " << filename.ascii() << " for reading\n";
    return false; 
  }
  if (!readPlotSet(istr, set)) { 
    std::cerr << "error reading plotset from " << filename.ascii() << "\n";
    return false; 
  }

  writePlotSet(std::cerr, set);

  mergePlotSet(set, clear);

  return true;

}

void PlotMainWindow::loadPlotSet() {

  QString s = 
    QFileDialog::getOpenFileName(QDir::currentDirPath(),
                                 "Plotset files (*.plots);; Any files (*)");

  if (s.isNull() || s.isEmpty()) { return; }

  mergePlotSet(s, true);

}

void PlotMainWindow::mergePlotSet() {

  QString s = 
    QFileDialog::getOpenFileName(QDir::currentDirPath(),
                                 "Plotset files (*.plots);; Any files (*)");

  if (s.isNull() || s.isEmpty()) { return; }

  mergePlotSet(s, false);

}


void PlotMainWindow::savePlotSet() {

  QString s = 
    QFileDialog::getSaveFileName(QDir::currentDirPath(),
                                 "Plotset files (*.plots);; Any files (*)");

  if (s.isNull() || s.isEmpty()) { return; }

  savePlotSet(s);

}

size_t PlotMainWindow::getFileIndex(const MzDataSet* dataset) const {

  size_t idx = 0;

  for (Q3ListViewItem* li = _datalist->firstChild(); li; li=li->nextSibling(), idx++) {
    MzDataSetItem* d = dynamic_cast<MzDataSetItem*>(li);
    if (!d) { continue; }
    if (d->dataset == dataset) { return idx; }
  }
       
  return size_t(-1);

  return 0;

}

const MzDataSet* PlotMainWindow::getDataSet(size_t index) const {

  size_t idx = 0;

  for (Q3ListViewItem* li = _datalist->firstChild(); li; li = li->nextSibling(), idx++) {
    if (idx == index) { 
      MzDataSetItem* d = dynamic_cast<MzDataSetItem*>(li);
      if (!d) { return 0; }
      return d->dataset;
    }
  }
       

  return 0;

}

MzDataSet* PlotMainWindow::getDataSet(size_t index) {

  size_t idx = 0;

  for (Q3ListViewItem* li = _datalist->firstChild(); li; li = li->nextSibling(), idx++) {
    if (idx == index) { 
      MzDataSetItem* d = dynamic_cast<MzDataSetItem*>(li);
      if (!d) { return 0; }
      return d->dataset;
    }
  }
       
  return 0;

}


void PlotMainWindow::closeEvent(QCloseEvent *e) {
  savePlotSet(defaultPlotSetFile());
  QWidget::closeEvent(e);
}

QString PlotMainWindow::defaultPlotSetFile() {
  return QDir::homeDirPath() + "/.mzplots";
}
