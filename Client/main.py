import os
import re
import sys
import time
import yaml

from PyQt5 import QtGui
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap, QIcon, QCursor
from PyQt5.QtMultimedia import QCameraInfo, QCamera, QCameraViewfinderSettings
from PyQt5.QtMultimediaWidgets import QCameraViewfinder
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QDesktopWidget, QMessageBox, \
    QDialogButtonBox
from PyQt5.uic import loadUi

from module import hid_def
from ui import main_ui

buffer = [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
mouse_buffer = [2, 0, 0, 0, 0, 0, 0, 0, 0]



class MyMainWindow(QMainWindow, main_ui.Ui_MainWindow):
    def __init__(self, parent=None):
        super(MyMainWindow, self).__init__(parent)
        self.setupUi(self)

        self.camera = None

        # 子窗口
        self.device_setup_dialog = loadUi("ui/device_setup_dialog.ui")
        self.device_setup_dialog.setWindowFlags(Qt.CustomizeWindowHint | Qt.WindowCloseButtonHint)

        self.shortcut_key_dialog = loadUi("ui/shortcut_key.ui")
        self.shortcut_key_dialog.setWindowFlags(Qt.CustomizeWindowHint | Qt.WindowCloseButtonHint)
        applybutton = self.shortcut_key_dialog.buttonBox.button(QDialogButtonBox.Apply)
        applybutton.clicked.connect(lambda: self.shortcut_key_func(1))
        applybutton.pressed.connect(lambda: self.shortcut_key_func(2))

        # 导入外部数据
        # with open("./Data/keyboard_hid2code.yaml", 'r') as load_f:
        #     self.keyboard_hid2code = yaml.safe_load(load_f)
        with open("Data/keyboard_scancode2hid.yml", 'r') as load_f:
            self.keyboard_scancode2hid = yaml.safe_load(load_f)
        with open("./Data/keyboard.yaml", 'r') as load_f:
            self.keyboard_code = yaml.safe_load(load_f)
        with open("./Data/config.yaml", 'r') as load_f:
            self.configfile = yaml.safe_load(load_f)
        # 加载配置文件
        self.camera_config = self.configfile['camera_config']
        self.config = self.configfile['config']
        self.status = {'fullscreen': False, 'mouse_capture': False, 'mouse_jumpframe': 0, 'init_ok': False,
                       'screen_height': 0, 'screen_width': 0}
        # 获取显示器分辨率大小
        self.desktop = QApplication.desktop()
        self.status['screen_height'] = self.desktop.screenGeometry().height()
        self.status['screen_width'] = self.desktop.screenGeometry().width()

        # 窗口图标
        self.setWindowIcon(QtGui.QIcon('ui/images/24/monitor-multiple.png'))
        self.device_setup_dialog.setWindowIcon(QtGui.QIcon('ui/images/24/import.png'))
        self.shortcut_key_dialog.setWindowIcon(QtGui.QIcon('ui/images/24/keyboard-settings-outline.png'))

        # 状态栏图标
        self.statusbar_lable1 = QLabel()
        self.statusbar_lable2 = QLabel()
        self.statusbar_lable3 = QLabel()
        self.statusbar_lable4 = QLabel()
        self.statusbar_lable1.setText("CTRL")
        self.statusbar_lable2.setText("SHIFT")
        self.statusbar_lable3.setText("ALT")
        self.statusbar_lable4.setText("META")
        self.statusbar_lable1.setStyleSheet('color: grey')
        self.statusbar_lable2.setStyleSheet('color: grey')
        self.statusbar_lable3.setStyleSheet('color: grey')
        self.statusbar_lable4.setStyleSheet('color: grey')
        self.statusBar().addPermanentWidget(self.statusbar_lable1)
        self.statusBar().addPermanentWidget(self.statusbar_lable2)
        self.statusBar().addPermanentWidget(self.statusbar_lable3)
        self.statusBar().addPermanentWidget(self.statusbar_lable4)

        self.statusbar_icon1 = QLabel()
        self.statusbar_icon2 = QLabel()
        self.statusbar_icon1.setPixmap(QPixmap('ui/images/24/video-off.png'))
        self.statusbar_icon2.setPixmap(QPixmap('ui/images/24/keyboard-off.png'))
        self.statusBar().addPermanentWidget(self.statusbar_icon1)
        self.statusBar().addPermanentWidget(self.statusbar_icon2)
        self.statusbar_icon1.setToolTip('Video capture card disconnect')
        self.statusbar_icon2.setToolTip('Keyboard Mouse device disconnect')

        font = QtGui.QFont()
        font.setFamily("Arial")
        self.statusBar().setStyleSheet("padding: 2px;")
        self.statusbar_lable1.setFont(font)
        self.statusbar_lable2.setFont(font)
        self.statusbar_lable3.setFont(font)
        self.statusbar_lable4.setFont(font)

        # self.statusbar_fill = QLabel()
        # self.statusBar().addPermanentWidget(self.statusbar_fill)

        # 菜单栏图标
        self.action_video_devices.setIcon(QIcon('ui/images/24/import.png'))
        self.action_video_device_connect.setIcon(QIcon('ui/images/24/video.png'))
        self.action_video_device_disconnect.setIcon(QIcon('ui/images/24/video-off.png'))
        self.actionMinimize.setIcon(QIcon('ui/images/24/window-minimize.png'))
        self.actionexit.setIcon(QIcon('ui/images/24/window-close.png'))
        self.actionReload_Key_Mouse.setIcon(QIcon('ui/images/24/reload.png'))
        self.action_fullscreen.setIcon(QIcon('ui/images/24/fullscreen.png'))
        self.action_Resize_window.setIcon(QIcon('ui/images/24/resize.png'))
        self.actionResetKeyboard.setIcon(QIcon('ui/images/24/reload.png'))
        self.actionResetMouse.setIcon(QIcon('ui/images/24/reload.png'))
        self.menuShortcut_key.setIcon(QIcon('ui/images/24/keyboard-outline.png'))
        self.actionCustomKey.setIcon(QIcon('ui/images/24/keyboard-settings-outline.png'))
        self.actionCapture_mouse.setIcon(QIcon('ui/images/24/mouse.png'))
        self.actionRelease_mouse.setIcon(QIcon('ui/images/24/mouse-off.png'))
        self.actionOn_screen_Keyboard.setIcon(QIcon('ui/images/24/keyboard-variant.png'))
        self.actionCalculator.setIcon(QIcon('ui/images/24/calculator.png'))
        self.actionSnippingTool.setIcon(QIcon('ui/images/24/monitor-screenshot.png'))
        self.actionNotepad.setIcon(QIcon('ui/images/24/notebook-edit.png'))
        self.actionIndicator_light.setIcon(QIcon('ui/images/24/led-diode.png'))

        # 遍历相机设备
        cameras = QCameraInfo()
        for i in cameras.availableCameras():
            self.camera_config['device_name'].append(i.description())
        print(self.camera_config['device_name'])

        # 初始化相机
        self.online_webcams = QCameraInfo.availableCameras()
        if self.online_webcams:
            self.camerafinder = QCameraViewfinder()
            self.setCentralWidget(self.camerafinder)
            # set the default webcam.
            try:
                self.get_webcam(self.camera_config['device_No'], self.camera_config['resolution_X'],
                                self.camera_config['resolution_Y'])
            except Exception as e:
                print(e)
                err = QMessageBox(self)
                err.setIcon(QMessageBox.Critical)
                err.setWindowTitle('Error')
                err.setText(str(e) + "\nPlease check the configuration file")
                err.exec()
                sys.exit(0)
            self.camerafinder.show()

        # 快捷键菜单设置快捷键名称
        self.actionq1.setText(self.configfile['shortcut_key']['shortcut_key_name'][0])
        self.actionq2.setText(self.configfile['shortcut_key']['shortcut_key_name'][1])
        self.actionq3.setText(self.configfile['shortcut_key']['shortcut_key_name'][2])
        self.actionq4.setText(self.configfile['shortcut_key']['shortcut_key_name'][3])
        self.actionq5.setText(self.configfile['shortcut_key']['shortcut_key_name'][4])
        self.actionq6.setText(self.configfile['shortcut_key']['shortcut_key_name'][5])
        self.actionq7.setText(self.configfile['shortcut_key']['shortcut_key_name'][6])
        self.actionq_8.setText(self.configfile['shortcut_key']['shortcut_key_name'][7])
        self.actionq9.setText(self.configfile['shortcut_key']['shortcut_key_name'][8])
        self.actionq10.setText(self.configfile['shortcut_key']['shortcut_key_name'][9])

        # 按键绑定
        self.action_video_device_connect.triggered.connect(lambda: self.set_webcam(True))
        self.action_video_device_disconnect.triggered.connect(lambda: self.set_webcam(False))
        self.action_video_devices.triggered.connect(self.device_config)
        self.actionCustomKey.triggered.connect(self.shortcut_key_func)
        self.actionReload_Key_Mouse.triggered.connect(lambda: self.reset_keymouse(4))
        self.actionMinimize.triggered.connect(self.window_minimized)
        self.actionexit.triggered.connect(sys.exit)

        self.device_setup_dialog.comboBox.currentIndexChanged.connect(self.update_device_setup_resolutions)

        self.action_fullscreen.triggered.connect(self.fullscreen_func)
        self.action_Resize_window.triggered.connect(self.resize_window_func)
        self.actionRelease_mouse.triggered.connect(self.release_mouse)
        self.actionCapture_mouse.triggered.connect(self.capture_mouse)
        self.actionResetKeyboard.triggered.connect(lambda: self.reset_keymouse(1))
        self.actionResetMouse.triggered.connect(lambda: self.reset_keymouse(3))
        self.actionIndicator_light.triggered.connect(self.indicatorLight_func)
        self.actionReload_MCU.triggered.connect(lambda: self.reset_keymouse(2))

        self.actionq1.triggered.connect(lambda: self.shortcut_key_action(0))
        self.actionq2.triggered.connect(lambda: self.shortcut_key_action(1))
        self.actionq3.triggered.connect(lambda: self.shortcut_key_action(2))
        self.actionq4.triggered.connect(lambda: self.shortcut_key_action(3))
        self.actionq5.triggered.connect(lambda: self.shortcut_key_action(4))
        self.actionq6.triggered.connect(lambda: self.shortcut_key_action(5))
        self.actionq7.triggered.connect(lambda: self.shortcut_key_action(6))
        self.actionq_8.triggered.connect(lambda: self.shortcut_key_action(7))
        self.actionq9.triggered.connect(lambda: self.shortcut_key_action(8))
        self.actionq10.triggered.connect(lambda: self.shortcut_key_action(9))

        self.actionOn_screen_Keyboard.triggered.connect(lambda: self.menu_tools_actions(0))
        self.actionCalculator.triggered.connect(lambda: self.menu_tools_actions(1))
        self.actionSnippingTool.triggered.connect(lambda: self.menu_tools_actions(2))
        self.actionNotepad.triggered.connect(lambda: self.menu_tools_actions(3))

        # 设置聚焦方式
        self.camerafinder.setFocusPolicy(Qt.ClickFocus)
        self.menubar.setFocusPolicy(Qt.ClickFocus)

        # 初始化hid设备
        self.reset_keymouse(4)
        # 创建指针对象
        self.cursor = QCursor()
        self.setMouseTracking(True)
        self.camerafinder.setMouseTracking(True)

        self.resize_window_func()
        self.status['init_ok'] = True

    # 弹出采集卡设备设置窗口，并打开采集卡设备
    def device_config(self):
        self.device_setup_dialog.comboBox.clear()

        # 遍历相机设备
        cameras = QCameraInfo()
        for i in cameras.availableCameras():
            self.camera_config['device_name'].append(i.description())
            self.device_setup_dialog.comboBox.addItem(i.description())

        # 将设备设置为字典camera_config中指定的设备
        self.device_setup_dialog.comboBox.setCurrentIndex(self.camera_config['device_No'])
        resolution_str = str(self.camera_config['resolution_X']) + 'x' + str(self.camera_config['resolution_Y'])
        print(resolution_str)
        self.device_setup_dialog.comboBox_2.setCurrentText(resolution_str)

        # 如果选择设备
        info = self.device_setup_dialog.exec()

        if info == 1:
            print(self.device_setup_dialog.comboBox.currentIndex())
            print(self.device_setup_dialog.comboBox_2.currentText().split('x'))

            self.camera_config['device_No'] = self.device_setup_dialog.comboBox.currentIndex()
            self.camera_config['resolution_X'] = int(self.device_setup_dialog.comboBox_2.currentText().split('x')[0])
            self.camera_config['resolution_Y'] = int(self.device_setup_dialog.comboBox_2.currentText().split('x')[1])

            print(self.camera_config)

            try:
                self.set_webcam(True)
                self.resize_window_func()
            except Exception as e:
                print(e)

    # 获取采集卡分辨率
    def update_device_setup_resolutions(self):
        self.device_setup_dialog.comboBox_2.clear()
        camera_info = QCamera(QCameraInfo.availableCameras()[self.device_setup_dialog.comboBox.currentIndex()])
        camera_info.load()
        for i in camera_info.supportedViewfinderResolutions():
            resolutions_str = str(i.width()) + 'x' + str(i.height())
            self.device_setup_dialog.comboBox_2.addItem(resolutions_str)
        print(camera_info.supportedViewfinderResolutions())
        camera_info.unload()

    # 初始化指定配置视频设备
    def get_webcam(self, i, x, y):
        self.camera = QCamera(self.online_webcams[i])
        # self.camera = QCamera()
        self.camera.setViewfinder(self.camerafinder)
        self.camera.setCaptureMode(QCamera.CaptureStillImage)
        self.camera.error.connect(lambda: self.alert(self.camera.errorString()))
        view_finder_settings = QCameraViewfinderSettings()
        view_finder_settings.setResolution(x, y)
        self.camera.setViewfinderSettings(view_finder_settings)

        if not self.status['fullscreen']:
            self.resize(x, y + 60)
        # self.camera.start()

    # 视频设备错误提示
    def alert(self, s):
        """
        This handle errors and displaying alerts.
        """
        err = QMessageBox(self)
        err.setWindowTitle('Error')
        err.setText(s)
        err.exec()
        self.device_event_handle("video_error")
        print(s)
        print(err)

    # 启用和禁用视频设备
    def set_webcam(self, s):
        if s:
            self.device_event_handle("video_ok")
            self.get_webcam(self.camera_config['device_No'], self.camera_config['resolution_X'],
                            self.camera_config['resolution_Y'])
            self.camera.start()
            self.camerafinder.setMouseTracking(True)
            # self.camera_config['connected'] = True
        else:
            self.device_event_handle("video_close")
            self.camera.stop()
            # print("video device disconnect")
            self.camerafinder.setMouseTracking(False)


    # 捕获鼠标功能
    def capture_mouse(self):
        self.status['mouse_capture'] = True

    # 释放鼠标功能
    def release_mouse(self):
        self.status['mouse_capture'] = False
        hidinfo = hid_def.hid_report([2, 0, 0, 0, 0, 0, 0, 0, 0])
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

    # 通过视频设备分辨率调整窗口大小
    def resize_window_func(self):
        self.resize(self.camera_config['resolution_X'], self.camera_config['resolution_Y'] + 60)
        self.camerafinder.setAspectRatioMode(Qt.IgnoreAspectRatio)
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

        if self.status['screen_height'] - self.height() < 100:
            self.showMaximized()

    # 最小化窗口
    def window_minimized(self):
        self.showMinimized()

    # 重置键盘鼠标
    def reset_keymouse(self, s):
        if s == 1:  # keyboard
            for i in range(2, len(buffer)):
                buffer[i] = 0
            hidinfo = hid_def.hid_report(buffer)
            if hidinfo == 1 or hidinfo == 4:
                self.device_event_handle("hid_error")
            elif hidinfo == 0:
                self.device_event_handle("hid_ok")
            self.shortcut_status(buffer)
        elif s == 2:  # MCU
            hidinfo = hid_def.hid_report([4, 0])
            if hidinfo == 1 or hidinfo == 4:
                self.device_event_handle("hid_error")
            elif hidinfo == 0:
                self.device_event_handle("hid_ok")
        elif s == 3:  # mouse
            for i in range(2, len(mouse_buffer)):
                mouse_buffer[i] = 0
            hidinfo = hid_def.hid_report(mouse_buffer)
            if hidinfo == 1 or hidinfo == 4:
                self.device_event_handle("hid_error")
            elif hidinfo == 0:
                self.device_event_handle("hid_ok")
        elif s == 4:  # hid
            hid_code = hid_def.init_usb(hid_def.vendor_id, hid_def.usage_page)
            if hid_code == 0:
                self.device_event_handle("hid_init_ok")
            else:
                self.device_event_handle("hid_init_error")

    # 自定义组合键窗口
    def shortcut_key_func(self, s):
        if s == 1:  # clicked
            hid_return = hid_def.hid_report(buffer)
            if hid_return != 1 or hid_return != 4:
                hid_def.hid_report(buffer)
                time.sleep(0.1)
                hid_def.hid_report([1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
            else:
                self.device_event_handle("hid_error")
            return
        if s == 2:  # pressed
            hid_return = hid_def.hid_report(buffer)
            if hid_return == 1 or hid_return == 4:
                self.device_event_handle("hid_error")
            return

        self.shortcut_key_dialog.pushButton_ctrl.clicked.connect(lambda: self.shortcut_key_handle(1))
        self.shortcut_key_dialog.pushButton_alt.clicked.connect(lambda: self.shortcut_key_handle(4))
        self.shortcut_key_dialog.pushButton_shift.clicked.connect(lambda: self.shortcut_key_handle(2))
        self.shortcut_key_dialog.pushButton_meta.clicked.connect(lambda: self.shortcut_key_handle(8))
        self.shortcut_key_dialog.pushButton_tab.clicked.connect(lambda: self.shortcut_key_handle(0x2b))
        self.shortcut_key_dialog.pushButton_prtsc.clicked.connect(lambda: self.shortcut_key_handle(0x46))
        self.shortcut_key_dialog.keySequenceEdit.keySequenceChanged.connect(lambda: self.shortcut_key_handle(0xff))
        self.shortcut_key_dialog.pushButton_clear.clicked.connect(lambda: self.shortcut_key_handle(0xfe))

        wm_pos = self.geometry()
        wm_size = self.size()
        self.shortcut_key_dialog.move(wm_pos.x() + (wm_size.width() - 542), wm_pos.y() + (wm_size.height() - 330))

        dialog_return = self.shortcut_key_dialog.exec()
        if dialog_return == 1:
            hidinfo = hid_def.hid_report(buffer)
            if hidinfo != 1 or hidinfo != 4:
                for i in range(2, len(buffer)):
                    buffer[i] = 0
                time.sleep(0.1)
                hid_def.hid_report(buffer)
                self.shortcut_key_handle(0xfe)
            else:
                self.device_event_handle("hid_error")
        elif dialog_return == 0:
            for i in range(2, len(buffer)):
                buffer[i] = 0
            self.shortcut_key_handle(0xfe)

    # 自定义组合键窗口按钮对应功能
    def shortcut_key_handle(self, s):
        shift_symbol = [")", "!", "@", "#", "$", "%", "^", "&", "*", "(", "~", "_", "+", "{", "}", "|", ":", "\"",
                        "<", ">", "?"]
        if s == 0xfe:
            self.shortcut_key_dialog.pushButton_ctrl.setChecked(False)
            self.shortcut_key_dialog.pushButton_alt.setChecked(False)
            self.shortcut_key_dialog.pushButton_shift.setChecked(False)
            self.shortcut_key_dialog.pushButton_meta.setChecked(False)
            self.shortcut_key_dialog.pushButton_tab.setChecked(False)
            self.shortcut_key_dialog.pushButton_prtsc.setChecked(False)
            self.shortcut_key_dialog.keySequenceEdit.setKeySequence("")
            for i in range(2, len(buffer)):
                buffer[i] = 0

        if s == 0xff:  # keySequenceChanged
            if self.shortcut_key_dialog.keySequenceEdit.keySequence().count() == 0:  # 去除多个复合键
                keysequence = ""
                return
            elif self.shortcut_key_dialog.keySequenceEdit.keySequence().count() == 1:
                keysequence = self.shortcut_key_dialog.keySequenceEdit.keySequence().toString()
            else:
                keysequence = self.shortcut_key_dialog.keySequenceEdit.keySequence().toString().split(",")
                self.shortcut_key_dialog.keySequenceEdit.setKeySequence(keysequence[0])
                keysequence = keysequence[0]

            if [s for s in shift_symbol if keysequence in s]:
                keysequence = "Shift+" + keysequence

            if len(re.findall("\+", keysequence)) == 0:  # 没有匹配到+号，不是组合键
                self.shortcut_key_dialog.keySequenceEdit.setKeySequence(keysequence)
            else:
                if keysequence != '+':
                    keysequence_list = keysequence.split("+").copy()  # 将复合键转换为功能键
                    if [s for s in keysequence_list if "Ctrl" in s]:
                        self.shortcut_key_dialog.pushButton_ctrl.setChecked(True)
                        buffer[2] = buffer[2] | 1
                    else:
                        self.shortcut_key_dialog.pushButton_ctrl.setChecked(False)

                    if [s for s in keysequence_list if "Alt" in s]:
                        self.shortcut_key_dialog.pushButton_alt.setChecked(True)
                        buffer[2] = buffer[2] | 4
                    else:
                        self.shortcut_key_dialog.pushButton_alt.setChecked(False)

                    if [s for s in keysequence_list if "Shift" in s]:
                        self.shortcut_key_dialog.pushButton_shift.setChecked(True)
                        buffer[2] = buffer[2] | 2
                    else:
                        self.shortcut_key_dialog.pushButton_shift.setChecked(False)

                    self.shortcut_key_dialog.keySequenceEdit.setKeySequence(keysequence_list[-1])
                    keysequence = keysequence_list[-1]
            try:
                mapcode = self.keyboard_code[keysequence.upper()]
            except Exception as e:
                print(e)
                print("Hid query error")
                return

            if not mapcode:
                print("Hid query error")
            else:
                buffer[4] = int(mapcode, 16)  # 功能位

        if self.shortcut_key_dialog.pushButton_ctrl.isChecked() and s == 1:
            buffer[2] = buffer[2] | 1
        elif (self.shortcut_key_dialog.pushButton_ctrl.isChecked() is False) and s == 1:
            buffer[2] = buffer[2] & 1 and buffer[2] ^ 1

        if self.shortcut_key_dialog.pushButton_alt.isChecked() and s == 4:
            buffer[2] = buffer[2] | 4
        elif (self.shortcut_key_dialog.pushButton_alt.isChecked() is False) and s == 4:
            buffer[2] = buffer[2] & 4 and buffer[2] ^ 4

        if self.shortcut_key_dialog.pushButton_shift.isChecked() and s == 2:
            buffer[2] = buffer[2] | 2
        elif (self.shortcut_key_dialog.pushButton_shift.isChecked() is False) and s == 2:
            buffer[2] = buffer[2] & 2 and buffer[2] ^ 2

        if self.shortcut_key_dialog.pushButton_meta.isChecked() and s == 8:
            buffer[2] = buffer[2] | 8
        elif (self.shortcut_key_dialog.pushButton_meta.isChecked() is False) and s == 8:
            buffer[2] = buffer[2] & 8 and buffer[2] ^ 8

        if self.shortcut_key_dialog.pushButton_tab.isChecked() and s == 0x2b:
            buffer[8] = 0x2b
        elif (self.shortcut_key_dialog.pushButton_tab.isChecked() is False) and s == 0x2b:
            buffer[8] = 0

        if self.shortcut_key_dialog.pushButton_prtsc.isChecked() and s == 0x46:
            buffer[9] = 0x46
        elif (self.shortcut_key_dialog.pushButton_prtsc.isChecked() is False) and s == 0x46:
            buffer[9] = 0

    # 菜单快捷键发送hid报文
    def shortcut_key_action(self, s):
        if -1 < s < 10:
            hid_def.hid_report(self.configfile['shortcut_key']['shortcut_key_hidcode'][s])
            time.sleep(0.1)
            hidinfo = hid_def.hid_report([1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
            if hidinfo == 1 or hidinfo == 4:
                self.device_event_handle("hid_error")

    # 设备事件处理
    def device_event_handle(self, s):
        if s == "hid_error":
            self.statusBar().showMessage("Keyboard Mouse connect error, try to <Reload Key/Mouse>")
            self.statusbar_icon2.setPixmap(QPixmap('ui/images/24/keyboard-off.png'))
            self.statusbar_icon2.setToolTip('Keyboard Mouse disconnect')
            self.status['mouse_capture'] = False
        elif s == "video_error":
            self.statusBar().showMessage("Video device error")
            self.statusbar_icon1.setPixmap(QPixmap('ui/images/24/video-off.png'))
            self.statusbar_icon1.setToolTip('Video capture card disconnect')
        elif s == "video_close":
            self.statusBar().showMessage("Video device close")
            self.statusbar_icon1.setPixmap(QPixmap('ui/images/24/video-off.png'))
            self.statusbar_icon1.setToolTip('Video capture card disconnect')
        elif s == "hid_init_error":
            self.statusBar().showMessage("Keyboard Mouse initialization error")
            self.statusbar_icon2.setPixmap(QPixmap('ui/images/24/keyboard-off.png'))
            self.statusbar_icon2.setToolTip('Keyboard Mouse initialization error')

        if s == "hid_init_ok":
            self.statusBar().showMessage("Keyboard Mouse initialization done")
            self.statusbar_icon2.setPixmap(QPixmap('ui/images/24/keyboard.png'))
            self.statusbar_icon2.setToolTip('Keyboard Mouse initialization done')
        elif s == "hid_ok":
            self.statusbar_icon2.setPixmap(QPixmap('ui/images/24/keyboard.png'))
            self.statusBar().addPermanentWidget(self.statusbar_icon2)
            self.statusbar_icon2.setToolTip('Keyboard Mouse device connected')
        elif s == "video_ok":
            self.statusBar().showMessage("Video device connected")
            self.statusbar_icon1.setPixmap(QPixmap('ui/images/24/video.png'))
            self.statusbar_icon1.setToolTip('Video capture card connected')
            self.status['mouse_capture'] = True

    # 菜单小工具
    def menu_tools_actions(self, s):
        if s == 0:
            os.popen('osk')
        elif s == 1:
            os.popen('calc')
        elif s == 2:
            os.popen('SnippingTool')
        elif s == 3:
            os.popen('notepad')

    # 状态栏显示组合键状态
    def shortcut_status(self, s):
        if s[2] & 1:
            self.statusbar_lable1.setStyleSheet('color: black')
        else:
            self.statusbar_lable1.setStyleSheet('color: grey')

        if s[2] & 2:
            self.statusbar_lable2.setStyleSheet('color: black')
        else:
            self.statusbar_lable2.setStyleSheet('color: grey')

        if s[2] & 4:
            self.statusbar_lable3.setStyleSheet('color: black')
        else:
            self.statusbar_lable3.setStyleSheet('color: grey')

        if s[2] & 8:
            self.statusbar_lable4.setStyleSheet('color: black')
        else:
            self.statusbar_lable4.setStyleSheet('color: grey')

    def indicatorLight_func(self):
        reply = hid_def.hid_report([3, 0], True)
        print(reply)
        if reply == 1 or reply == 2 or reply == 3 or reply == 4:
            self.device_event_handle("hid_error")
            print("hid error")
            return

        if reply[0] != 3:
            messageBoxText = "reply error"
        else:
            messageBoxText = \
                "[" + ((reply[2] & (1 << 0)) and "*" or "  ") + "] Num Lock     " "[" + (
                        (reply[2] & (1 << 1)) and "*" or "  ") + "] Caps Lock     ""[" + (
                        (reply[2] & (1 << 2)) and "*" or "  ") + "] Scroll Lock     "

        err = QMessageBox(self)
        err.setWindowTitle('Indicator Light')
        err.setText(messageBoxText)
        err.exec()

    # 全屏幕切换
    def fullscreen_func(self):
        self.status['fullscreen'] = ~ self.status['fullscreen']
        if self.status['fullscreen']:
            self.showFullScreen()
            self.action_fullscreen.setChecked(True)
            self.action_Resize_window.setEnabled(False)
        else:
            self.showNormal()
            self.action_fullscreen.setChecked(False)
            self.action_Resize_window.setEnabled(True)

    # 窗口失焦事件
    def changeEvent(self, event):
        try:  # 窗口未初始化完成时会触发一次事件
            if self.status is None:
                return
        except AttributeError:
            print("status Variable not initialized")
        except Exception as e:
            print(e)
        else:
            if not self.isActiveWindow() and self.status['init_ok']:  # 窗口失去焦点时重置键盘，防止卡键
                # print("window not active")
                self.reset_keymouse(1)


    # 鼠标按下事件
    def mousePressEvent(self, event):
        if not self.status['mouse_capture']:
            return
        if event.button() == Qt.LeftButton:
            mouse_buffer[2] = mouse_buffer[2] | 1
        elif event.button() == Qt.RightButton:
            mouse_buffer[2] = mouse_buffer[2] | 2
            # mouse_buffer = [2, 0, 2, 0, 0, 0, 0, 0, 0]
        elif event.button() == Qt.MidButton:
            mouse_buffer[2] = mouse_buffer[2] | 4

        hidinfo = hid_def.hid_report(mouse_buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

    # 鼠标松开事件
    def mouseReleaseEvent(self, event):
        if not self.status['mouse_capture']:
            return
        if event.button() == Qt.LeftButton and mouse_buffer[2] & 1:
            mouse_buffer[2] = mouse_buffer[2] ^ 1
        elif event.button() == Qt.RightButton and mouse_buffer[2] & 2:
            mouse_buffer[2] = mouse_buffer[2] ^ 2
        elif event.button() == Qt.MidButton and mouse_buffer[2] & 4:
            mouse_buffer[2] = mouse_buffer[2] ^ 4

        if mouse_buffer[2] < 0 or mouse_buffer[2] > 7:
            mouse_buffer[2] = 0
        hidinfo = hid_def.hid_report(mouse_buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

    # 鼠标滚动事件
    def wheelEvent(self, event):
        mouse_wheel_buffer = [2, 0, 0, 0, 0, 0, 0, 0, 0]
        if not self.status['mouse_capture']:
            return
        if event.angleDelta().y() == 120:
            mouse_wheel_buffer[7] = 0x01
        elif event.angleDelta().y() == -120:
            mouse_wheel_buffer[7] = 0xff
        else:
            mouse_wheel_buffer[7] = 0

        hidinfo = hid_def.hid_report(mouse_wheel_buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")
        time.sleep(0.006)
        mouse_wheel_buffer[7] = 0
        hidinfo = hid_def.hid_report(mouse_wheel_buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

    # 鼠标移动事件
    def mouseMoveEvent(self, event):
        if not self.status['mouse_capture']:
            return

        x, y = event.pos().x(), event.pos().y()
        info = 'X=' + str(x) + ' Y=' + str(y)
        self.statusBar().showMessage(info)

        x_hid = int((32767 * x) / self.camerafinder.width())
        y_hid = int((32767 * (y - self.camerafinder.pos().y())) / self.camerafinder.height())

        mouse_buffer[3] = x_hid & 0xff
        mouse_buffer[4] = x_hid >> 8
        mouse_buffer[5] = y_hid & 0xff
        mouse_buffer[6] = y_hid >> 8
        hidinfo = hid_def.hid_report(mouse_buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

    # 键盘按下事件
    def keyPressEvent(self, event):
        if event.nativeScanCode() == 285:
            self.release_mouse()
        if event.isAutoRepeat():
            return
        key = event.key()
        scancode = event.nativeScanCode()

        scancode2hid = self.keyboard_scancode2hid.get(scancode, 0)
        if scancode2hid != 0:
            buffer[4] = scancode2hid
        else:
            buffer[4] = 0

        if key == Qt.Key_Control:  # Ctrl 键被按下
            buffer[2] = buffer[2] | 1
        elif key == Qt.Key_Shift:  # Shift 键被按下
            buffer[2] = buffer[2] | 2
        elif key == Qt.Key_Alt:  # Alt 键被按下
            buffer[2] = buffer[2] | 4
        elif key == Qt.Key_Meta:  # Meta 键被按下
            buffer[2] = buffer[2] | 8

        if buffer[2] < 0 or buffer[2] > 16:
            buffer[2] = 0

        hidinfo = hid_def.hid_report(buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")
        self.shortcut_status(buffer)

        '''
        其它常用按键：
        Qt.Key_Escape,Qt.Key_Tab,Qt.Key_Backspace,Qt.Key_Return,Qt.Key_Enter,
        Qt.Key_Insert,Qt.Key_Delet###.Key_9,Qt.Key_Colon,Qt.Key_Semicolon,Qt.Key_Equal
        ...
        '''

    # 键盘松开事件
    def keyReleaseEvent(self, event):
        if event.isAutoRepeat():
            return

        key = event.key()
        scancode = event.nativeScanCode()
        scancode2hid = self.keyboard_scancode2hid.get(scancode, 0)

        if scancode2hid != 0:
            buffer[4] = 0

        if key == Qt.Key_Control:  # Ctrl 键被释放
            if buffer[2] & 1:
                buffer[2] = buffer[2] ^ 1
        elif key == Qt.Key_Shift:  # Shift 键被释放
            if buffer[2] & 2:
                buffer[2] = buffer[2] ^ 2
        elif key == Qt.Key_Alt:  # Alt 键被释放
            if buffer[2] & 4:
                buffer[2] = buffer[2] ^ 4
        elif key == Qt.Key_Meta:  # Meta 键被释放
            if buffer[2] & 8:
                buffer[2] = buffer[2] ^ 8

        if buffer[2] < 0 or buffer[2] > 16:
            buffer[2] = 0

        hidinfo = hid_def.hid_report(buffer)
        if hidinfo == 1 or hidinfo == 4:
            self.device_event_handle("hid_error")

        self.shortcut_status(buffer)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    myWin = MyMainWindow()
    myWin.show()
    sys.exit(app.exec_())
