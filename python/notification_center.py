#! /usr/local/bin/python

# Qt
from PyQt4 import QtCore
from PyQt4 import QtGui

# System
import sys

# Studio
import studioenv
import studio

# import the studio modules
from studio import application
from studio import notification_center

# Global signal
TRIGGERED_SIG =  QtCore.SIGNAL("triggered()")

kEventId = notification_center.EventId("testEvent")

# MainWindow class
class MainWindow(QtGui.QMainWindow):

    def __init__(self, title):
        
        QtGui.QMainWindow.__init__(self)

        self.setMinimumSize(800,500)
        self.setWindowTitle(title)

        self.createMenus()
        
    def registerNotification(self):    
        #notificationCenter.registerEvent(kEventId)
        id1 = notificationCenter.connect(kEventId, self.test)
        id2 = notificationCenter.connect(kEventId, self.test1)
        notificationCenter.disconnect(id1)
        notificationCenter.registerEvent(kEventId)

    def postNotification(self):
        notificationCenter.sendEvent(notification_center.Event(kEventId))
        
    def test(self, event):
        print "test ", event
        
    def test1(self, event):
        print "test1 ", event        

        # build file and create menus
    def createMenus(self):

        # Create the top level menubar
        menubar = QtGui.QMenuBar(self)

        # Create the File menu         
        menuFile = QtGui.QMenu(menubar)
        menuFile.setTitle("File")

        # Add the File menu items
    
        # Create the Quit item
        quitAction = QtGui.QAction(self)
        quitAction.setText("Quit")
        quitAction.setShortcut(QtGui.QKeySequence("Ctrl+Q"))        
        QtCore.QObject.connect(quitAction, TRIGGERED_SIG, QtGui.QApplication.quit)                              
        menuFile.addAction(quitAction)
        menubar.addAction(menuFile.menuAction())

        # Create the Test menu
        menuTest = QtGui.QMenu(menubar)
        menuTest.setTitle("Notifications")

        # Create the notification test items            

        # Create the Register item
        registerNotificationAction = QtGui.QAction(self)
        registerNotificationAction.setText("Register Notification")
        QtCore.QObject.connect(registerNotificationAction, TRIGGERED_SIG, self.registerNotification)                              
        menuTest.addAction(registerNotificationAction)
        menubar.addAction(menuTest.menuAction())

        # Create the Post item
        postNotificationAction = QtGui.QAction(self)
        postNotificationAction.setText("Post Notification")
        QtCore.QObject.connect(postNotificationAction, TRIGGERED_SIG, self.postNotification)                              
        menuTest.addAction(postNotificationAction)
        menubar.addAction(menuTest.menuAction())
         
        # Add the menubar to the window
        self.setMenuBar(menubar)


if __name__ == "__main__":
    app = application.Application(sys.argv, "notification_test")

    #app.notificationCenter = notification_center.NotificationCenter.instance()
    notificationCenter = notification_center.NotificationCenter.instance();

    window = MainWindow("Notification Center");
    window.show()

    app.exec_()
