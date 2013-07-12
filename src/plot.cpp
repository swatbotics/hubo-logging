#include "PlotMainWindow.h"
#include <QApplication>
#include <iostream>

int main(int argc, char** argv) {

  QApplication app(argc, argv);
  app.connect(&app, SIGNAL(lastWindowClosed()), SLOT(quit()));

  PlotMainWindow w;
  std::vector<QString> plotsets;

  for (int i=1; i<argc; ++i) {
    QString str = argv[i];
    if (str.endsWith(".plots")) {
      plotsets.push_back(str);
    } else {
      w.openData(str);
    }
  }

  if (!w.getDataSet(0)) { w.openData("latest.data"); }

  if (plotsets.empty()) {
    w.mergePlotSet(PlotMainWindow::defaultPlotSetFile(), true);
  } else {
    for (size_t i=0; i<plotsets.size(); ++i) {
      w.mergePlotSet(plotsets[i], false);
    }
  }

  w.show();
  return app.exec();

};
