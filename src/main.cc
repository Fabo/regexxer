
#include <gtkmm/main.h>
#include "mainwindow.h"

int main(int argc, char** argv)
{
  Gtk::Main main_instance (&argc, &argv);

  Regexxer::MainWindow window;
  Gtk::Main::run(window);

  return 0;
}

