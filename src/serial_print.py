import os
import serial
import time
import datetime
import wmi
import pythoncom
import win32serviceutil
import win32service
import win32event
import servicemanager
import socket

class AppServerSvc(win32serviceutil.ServiceFramework):
    _svc_name_ = 'LCDStatus'
    _svc_display_name_ = 'Arduino LCD Status'

    stop_flag = False

    def __init__(self,args):
        win32serviceutil.ServiceFramework.__init__(self,args)
        self.hWaitStop = win32event.CreateEvent(None,0,0,None)
        socket.setdefaulttimeout(60)

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
        win32event.SetEvent(self.hWaitStop)
        self.stop_flag = True

    def SvcDoRun(self):
        servicemanager.LogMsg(servicemanager.EVENTLOG_INFORMATION_TYPE,
                              servicemanager.PYS_SERVICE_STARTED,
                              (self._svc_name_,''))
        self.main()

    def main(self):
        try:
            w = wmi.WMI()
            com = serial.Serial('COM3', baudrate=9600, timeout=1)
            time.sleep(2.5) # allows time for the serial connection to establish
            
            com.write("\nTIf a line is longer than 16 characters it will scroll to display itself.".encode())

            while True:

                if self.stop_flag:
                    break

                c = w.Win32_OperatingSystem()[0]
                m = c.wmi_property('FreePhysicalMemory')
                mem = float(m.value)/2**10 # convert to MB

                c = w.CIM_Processor()[0]
                m = c.wmi_property('CurrentClockSpeed')
                cpu_clock = int(m.value) # Mhz

                c = w.Win32_Processor()[0]
                m = c.wmi_property('LoadPercentage')
                cpu_load = m.value

                now = datetime.datetime.now()
                tstr = now.strftime("%H:%M")

                if cpu_load!=None:
                    com.write((("\nTCPU %02d%%  %04dMHz")%(cpu_load,cpu_clock)).encode())
                    
                string = ("\nBMEM %4.0fMB "%mem)+tstr
                com.write(string.encode())
                
                time.sleep(0.25)
                
        finally:
            com.write("\nBClosing serial connection.".encode())
            com.close()
            self.ReportServiceStatus(win32service.SERVICE_STOPPED)
    # end main

if __name__ == '__main__':
    win32serviceutil.HandleCommandLine(AppServerSvc)

