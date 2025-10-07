// <copyright file="GUIsimpleForm.cs" company="Hottinger Baldwin Messtechnik GmbH">
//
// WTXGUIsimple, a demo application for HBM Weighing-API  
//
// The MIT License (MIT)
//
// Copyright (C) Hottinger Baldwin Messtechnik GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// </copyright>

namespace Hbm.Automation.Api.Weighing.Examples.GUIsimple
{
    using Hbm.Automation.Api.Data;
    using Hbm.Automation.Api.Weighing;
    using Hbm.Automation.Api.Weighing.DSE;
    using Hbm.Automation.Api.Weighing.DSE.Jet;
    using Hbm.Automation.Api.Weighing.Examples.GUISimple; // For AdjustmentCalculator and AdjustmentWeigher
    using System;
    using System.Windows.Forms;

    /// <summary>
    /// This application example demonstrates the usage of HBM Weighing-API.
    /// It shows how to connect a DSE device via Jet, how to get weight values, to calibrate and how to adjust the scale.
    /// </summary>
    public partial class GUIsimpleForm : Form
    {
        #region ==================== constants & fields ====================

        private const string DEFAULT_IP_ADDRESS = "192.168.100.88";
        private const string MESSAGE_CONNECTION_FAILED = "Connection failed!";
        private const string MESSAGE_CONNECTING = "Connecting...";
        private const int WAIT_DISCONNECT = 2000;

        private static BaseWTDevice _wtxDevice;
        private AdjustmentCalculator _adjustmentCalculator;
        private AdjustmentWeigher _adjustmentWeigher;
        private string _ipAddress = DEFAULT_IP_ADDRESS;
        private int _timerInterval = 200;
        private DSEJetConnection _jetConnection; // keep reference to hook logs

        #endregion

        #region =============== constructors & destructors =================

        /// <summary>
        /// Constructor of GUIsimpleForm
        /// </summary>
        /// <param name="args">Parameter from command line</param>
        public GUIsimpleForm(string[] args)
        {
            InitializeComponent();
            DisplayText("Check IP address and press 'Connect'.");
            EvaluateCommandLine(args);
            txtIPAddress.Text = _ipAddress;
            // Force device type combo to DSE (Jet) only
            if (this.cboDeviceType.Items.Count > 0)
            {
                this.cboDeviceType.SelectedIndex = 0;
                this.cboDeviceType.Enabled = false;
            }
            picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisPassive;
        }
        #endregion

        #region =============== protected & private methods ================

        /// <summary>
        /// Initialize a DSE Jet connection and create the device
        /// </summary>
        private void InitializeConnection()
        {
            this._ipAddress = txtIPAddress.Text;

            try
            {
                // Creating objects of DSEJetConnection:
                _jetConnection = new DSEJetConnection(_ipAddress);

                // surface communication logs to UI
                _jetConnection.CommunicationLog += (s, e) =>
                {
                    try { this.BeginInvoke(new Action(() => AppendInfo($"LOG: {e.Args}"))); } catch { }
                };

                // honor configured timer interval
                _wtxDevice = new DSEJet(_jetConnection, Math.Max(50, _timerInterval), update);

                // Connection establishment via Jet
                _wtxDevice.Connect(5000);
            }
            catch (System.IO.FileNotFoundException fnf)
            {
                picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisPassive;
                DisplayText($"{MESSAGE_CONNECTION_FAILED}{Environment.NewLine}{fnf.Message}{Environment.NewLine}Dica: adicione os pacotes 'SharpJet' e 'Newtonsoft.Json' também no projeto GUIsimple.");
                return;
            }
            catch (Exception ex)
            {
                picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisPassive;
                DisplayText($"{MESSAGE_CONNECTION_FAILED} {Environment.NewLine}{ex.Message}");
                if (ex.InnerException != null)
                {
                    AppendInfo($"Inner: {ex.InnerException.Message}");
                }
            }

            if (_wtxDevice != null && _wtxDevice.IsConnected)
            {
                picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisActive;
                GUISimple.Properties.Settings.Default.IPAddress = this._ipAddress;
                GUISimple.Properties.Settings.Default.Save();
            }
            else
            {
                picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisPassive;
                if (txtInfo.Text.IndexOf(MESSAGE_CONNECTION_FAILED, StringComparison.OrdinalIgnoreCase) < 0)
                {
                    DisplayText(MESSAGE_CONNECTION_FAILED);
                }
            }

        }

        /// <summary>
        /// How to get process data automatically
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">Here you find the current process data (e.g. weight value)</param>
        private void update(object sender, ProcessDataReceivedEventArgs e)
        {
            this.BeginInvoke(new Action(() =>
            {
                if (_wtxDevice == null || !_wtxDevice.IsConnected)
                {
                    return;
                }

                DisplayText("Net:" + _wtxDevice.PrintableWeight.Net + _wtxDevice.Unit + Environment.NewLine
                + "Gross:" + _wtxDevice.PrintableWeight.Gross + _wtxDevice.Unit + Environment.NewLine
                + "Tara:" + _wtxDevice.PrintableWeight.Tare + _wtxDevice.Unit);

                if (e.ProcessData.Underload == true)
                {
                    DisplayText("Underload : Lower than minimum" + Environment.NewLine);
                    picNE107.Image = GUISimple.Properties.Resources.NE107_OutOfSpecification;

                }
                else if (e.ProcessData.Overload == true)
                {
                    DisplayText("Overload : Higher than maximum capacity" + Environment.NewLine);
                    picNE107.Image = GUISimple.Properties.Resources.NE107_OutOfSpecification;

                }
                else if (e.ProcessData.HigherSafeLoadLimit == true)
                {
                    DisplayText("Higher than safe load limit" + Environment.NewLine);
                    picNE107.Image = GUISimple.Properties.Resources.NE107_OutOfSpecification;
                }
                else
                    picNE107.Image = GUISimple.Properties.Resources.NE107_DiagnosisActive;
            }));
        }

        /// <summary>
        /// Command line control
        /// </summary>
        /// <param name="args">Possible arguments: ip address, timer interval</param>
        private void EvaluateCommandLine(string[] args)
        {
            // Device type is fixed to DSE (Jet)
            if (args.Length > 0)
            {
                // If first arg is not an IP, treat it as IP anyway for simplicity
                _ipAddress = args[0];
            }
            else
            {
                _ipAddress = GUISimple.Properties.Settings.Default.IPAddress;
            }

            if (args.Length > 1)
                this._timerInterval = Convert.ToInt32(args[1]);
        }

        /// <summary>
        /// Displays the string, the measured values, unit etc. 
        /// </summary>
        /// <param name="text"></param>
        private void DisplayText(string text)
        {
            txtInfo.Text = text;
            Application.DoEvents();
        }

        private void AppendInfo(string text)
        {
            txtInfo.AppendText(Environment.NewLine + text);
            Application.DoEvents();
        }

        /// <summary>
        /// Connects to device
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cmdConnect_Click(object sender, EventArgs e)
        {
            if (_wtxDevice != null)
            {
                DisplayText("Disconnecting...");
                _wtxDevice.Connection.Disconnect();
                _wtxDevice = null;
            }

            DisplayText(MESSAGE_CONNECTING);
            this.InitializeConnection();
        }

        /// <summary>
        /// button click event for switching to gross or net value. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cmdGrossNet_Click(object sender, EventArgs e)
        {
            if (_wtxDevice == null || !_wtxDevice.IsConnected)
            {
                DisplayText("Device not connected.");
                return;
            }
            _wtxDevice.SetGross();
        }

        /// <summary>
        /// button click event for zeroing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cmdZero_Click(object sender, EventArgs e)
        {
            if (_wtxDevice == null || !_wtxDevice.IsConnected)
            {
                DisplayText("Device not connected.");
                return;
            }
            _wtxDevice.Zero();
        }

        /// <summary>
        /// button click event for taring 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cmdTare_Click(object sender, EventArgs e)
        {
            if (_wtxDevice == null || !_wtxDevice.IsConnected)
            {
                DisplayText("Device not connected.");
                return;
            }
            _wtxDevice.Tare();
        }

        //Method for calculate adjustment with dead load and span: 
        private void calibrationWithWeightToolStripMenuItem_Click_1(object sender, EventArgs e)
        {
            if (_wtxDevice != null)
            {
                _adjustmentCalculator = new AdjustmentCalculator(_wtxDevice);
                DialogResult res = _adjustmentCalculator.ShowDialog();
            }
        }

        /// <summary>
        /// Adjustment with weight
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void calibrationToolStripMenuItem_Click_1(object sender, EventArgs e)
        {
            if (_wtxDevice != null)
            {
                _adjustmentWeigher = new AdjustmentWeigher(_wtxDevice);
                DialogResult res = _adjustmentWeigher.ShowDialog();
            }
        }

        #endregion

        private void button1_Click(object sender, EventArgs e)
        {
            if (_wtxDevice == null || !_wtxDevice.IsConnected)
            {
                DisplayText("Device not connected.");
                return;
            }

            string display = "";
            switch (comboBox1.SelectedItem)
            {
                case "Serial number":
                    display = _wtxDevice.SerialNumber;
                    break;
                case "Device identification":
                    display = _wtxDevice.Identification;
                    break;
                case "Firmware version":
                    display = _wtxDevice.FirmwareVersion;
                    break;
                case "Weight step (DSE)":
                    display = ((DSEJet)_wtxDevice).WeightStep.ToString() + " " + _wtxDevice.Unit;
                    break;
                case "Scale range":
                    display = _wtxDevice.ScaleRange.ToString();
                    break;
                case "Tare mode":
                    display = _wtxDevice.TareMode.ToString();
                    break;
                case "Weight stable":
                    display = _wtxDevice.WeightStable.ToString();
                    break;
                case "Manual tare value":
                    display = _wtxDevice.ManualTareValue.ToString() + " " + _wtxDevice.Unit;
                    break;
                case "Maximum capacity":
                    display = _wtxDevice.MaximumCapacity.ToString() + " " + _wtxDevice.Unit;
                    break;
                case "Calibration weight":
                    display = _wtxDevice.CalibrationWeight.ToString() + " " + _wtxDevice.Unit;
                    break;
                case "LDW - Zero signal":
                    display = _wtxDevice.ZeroSignal.ToString() + " nV/V";
                    break;
                case "LWT - Nominal signal":
                    display = _wtxDevice.NominalSignal.ToString() + " nV/V";
                    break;
                case "Connection type":
                    display = _wtxDevice.ConnectionType;
                    break;
                case "Application mode":
                    display = _wtxDevice.ApplicationMode.ToString();
                    break;
                case "Zero value":
                    display = _wtxDevice.ZeroValue.ToString() + " " + _wtxDevice.Unit;
                    break;
            }
            textBox1.Text = display;
        }
    }
}
