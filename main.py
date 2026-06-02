import sys
import serial
import logging

logging.getLogger().setLevel(logging.ERROR)

from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *

from qt_material import apply_stylesheet


ROOMS = [
    "Bedroom1","Bedroom2","Bedroom3",
    "Living1","Living2",
    "Bathroom1","Bathroom2"
]


class HVACWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Smart HVAC Control Panel")

        self.selected_room = 0

        self.rooms_temp = [25]*7
        self.rooms_humi = [60]*7

        try:
            self.ser = serial.Serial("COM3",9600,timeout=1)
            self.status = "Connected COM3"
        except:
            self.ser = None
            self.status = "Serial Not Connected"

        self.init_ui()


    def init_ui(self):

        main_widget = QWidget()
        main_layout = QVBoxLayout()

        title = QLabel("智能暖通空调控制面板")
        title.setAlignment(Qt.AlignCenter)
        title.setStyleSheet("""
        font-size:40px;
        font-weight:bold;
        padding:20px;
        color:#2C3E50;
        """)

        main_layout.addWidget(title)

        # 房间区域
        self.room_grid = QGridLayout()
        self.room_buttons = []

        for i,name in enumerate(ROOMS):

            btn = QPushButton()
            btn.setMinimumSize(300,150)

            btn.clicked.connect(lambda _,x=i:self.select_room(x))

            self.room_buttons.append(btn)

            r = i//4
            c = i%4

            self.room_grid.addWidget(btn,r,c)

        main_layout.addLayout(self.room_grid)

        # 控制区域
        control_layout = QHBoxLayout()

        # 减号按钮样式
        minus_style = """
        QPushButton{
            background-color:#E67E22;
            color:white;
            font-size:36px;
            font-weight:bold;
            border-radius:15px;
        }
        QPushButton:hover{
            background-color:#D35400;
        }
        """

        # 加号按钮样式
        plus_style = """
        QPushButton{
            background-color:#2ECC71;
            color:white;
            font-size:36px;
            font-weight:bold;
            border-radius:15px;
        }
        QPushButton:hover{
            background-color:#27AE60;
        }
        """

        # 温度控制
        temp_layout = QVBoxLayout()

        temp_title = QLabel("Temperature")
        temp_title.setAlignment(Qt.AlignCenter)
        temp_title.setStyleSheet("font-size:22px;color:#2C3E50")

        self.temp_display = QLabel("25 °C")
        self.temp_display.setAlignment(Qt.AlignCenter)
        self.temp_display.setStyleSheet("""
        font-size:60px;
        font-weight:bold;
        color:#E67E22;
        """)

        temp_btn_layout = QHBoxLayout()

        temp_minus = QPushButton("−")
        temp_plus = QPushButton("+")

        temp_minus.setMinimumSize(120,80)
        temp_plus.setMinimumSize(120,80)

        temp_minus.setStyleSheet(minus_style)
        temp_plus.setStyleSheet(plus_style)

        temp_minus.clicked.connect(self.temp_down)
        temp_plus.clicked.connect(self.temp_up)

        temp_btn_layout.addWidget(temp_minus)
        temp_btn_layout.addWidget(self.temp_display)
        temp_btn_layout.addWidget(temp_plus)

        temp_layout.addWidget(temp_title)
        temp_layout.addLayout(temp_btn_layout)

        # 湿度控制
        humi_layout = QVBoxLayout()

        humi_title = QLabel("Humidity")
        humi_title.setAlignment(Qt.AlignCenter)
        humi_title.setStyleSheet("font-size:22px;color:#2C3E50")

        self.humi_display = QLabel("60 %")
        self.humi_display.setAlignment(Qt.AlignCenter)
        self.humi_display.setStyleSheet("""
        font-size:60px;
        font-weight:bold;
        color:#3498DB;
        """)

        humi_btn_layout = QHBoxLayout()

        humi_minus = QPushButton("−")
        humi_plus = QPushButton("+")

        humi_minus.setMinimumSize(120,80)
        humi_plus.setMinimumSize(120,80)

        humi_minus.setStyleSheet(minus_style)
        humi_plus.setStyleSheet(plus_style)

        humi_minus.clicked.connect(self.humi_down)
        humi_plus.clicked.connect(self.humi_up)

        humi_btn_layout.addWidget(humi_minus)
        humi_btn_layout.addWidget(self.humi_display)
        humi_btn_layout.addWidget(humi_plus)

        humi_layout.addWidget(humi_title)
        humi_layout.addLayout(humi_btn_layout)

        control_layout.addLayout(temp_layout)
        control_layout.addLayout(humi_layout)

        main_layout.addLayout(control_layout)

        # 状态栏
        self.status_label = QLabel(self.status)
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("""
        font-size:18px;
        padding:10px;
        color:#7F8C8D;
        """)

        main_layout.addWidget(self.status_label)

        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)

        self.refresh_ui()


    def refresh_ui(self):

        for i,name in enumerate(ROOMS):

            t = self.rooms_temp[i]
            h = self.rooms_humi[i]

            text = f"{name}\n\n{t}°C   {h}%"

            if i == self.selected_room:

                style = """
                background:#00ACC1;
                color:white;
                font-size:22px;
                font-weight:bold;
                border-radius:15px;
                """

            else:

                style = """
                background:#FFFFFF;
                color:#2C3E50;
                font-size:22px;
                border-radius:15px;
                """

            self.room_buttons[i].setText(text)
            self.room_buttons[i].setStyleSheet(style)

        self.temp_display.setText(f"{self.rooms_temp[self.selected_room]} °C")
        self.humi_display.setText(f"{self.rooms_humi[self.selected_room]} %")


    def select_room(self,i):

        self.selected_room = i
        self.refresh_ui()


    def temp_up(self):

        self.rooms_temp[self.selected_room]+=1
        self.send_temp()
        self.refresh_ui()


    def temp_down(self):

        self.rooms_temp[self.selected_room]-=1
        self.send_temp()
        self.refresh_ui()


    def humi_up(self):

        self.rooms_humi[self.selected_room]+=1
        self.send_humi()
        self.refresh_ui()


    def humi_down(self):

        self.rooms_humi[self.selected_room]-=1
        self.send_humi()
        self.refresh_ui()


    def send_temp(self):

        if not self.ser:
            return

        temp=int(self.rooms_temp[self.selected_room]*10)

        high=(temp>>8)&0xff
        low=temp&0xff

        frame=bytes([0xAA,0x55,0x01,self.selected_room+1,high,low])

        self.ser.write(frame)


    def send_humi(self):

        if not self.ser:
            return

        humi=self.rooms_humi[self.selected_room]

        high=(humi>>8)&0xff
        low=humi&0xff

        frame=bytes([0xAA,0x55,0x02,self.selected_room+1,high,low])

        self.ser.write(frame)



if __name__ == "__main__":

    app = QApplication(sys.argv)

    apply_stylesheet(app, theme='light_cyan.xml')

    window = HVACWindow()

    window.showMaximized()

    sys.exit(app.exec_())