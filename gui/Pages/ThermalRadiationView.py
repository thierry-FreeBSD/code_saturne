# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2014 EDF S.A.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301, USA.

#-------------------------------------------------------------------------------

"""
This module defines the Thermal Radiation model

This module contains the following classes and function:
- ThermalRadiationView
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------
import sys
if sys.version_info[0] == 2:
    import sip
    sip.setapi('QString', 2)

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Toolbox import GuiParam
from Pages.ThermalRadiationForm import Ui_ThermalRadiationForm
from Pages.ThermalRadiationAdvancedDialogForm import Ui_ThermalRadiationAdvancedDialogForm
import Base.QtPage as QtPage
from Pages.ThermalRadiationModel import ThermalRadiationModel
from Pages.OutputControlModel import OutputControlModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("ThermalRadiationView")
log.setLevel(GuiParam.DEBUG)

#--------------------------------------------------------------------------------
# Popup Class
#--------------------------------------------------------------------------------

class ThermalRadiationAdvancedDialogView(QDialog, Ui_ThermalRadiationAdvancedDialogForm):
    """
    Building of popup window for advanced options.
    """
    def __init__(self, parent, case, default):
        """
        Constructor
        """
        QDialog.__init__(self, parent)

        Ui_ThermalRadiationAdvancedDialogForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.case.undoStopGlobal()

        self.setWindowTitle(self.tr("Advanced options"))
        self.default = default
        self.result  = self.default.copy()

        # Combo models

        self.modelTSRay  = QtPage.ComboModel(self.comboBoxTSRay, 3, 1)
        self.modelPrintT = QtPage.ComboModel(self.comboBoxPrintT, 3, 1)
        self.modelPrintL = QtPage.ComboModel(self.comboBoxPrintL, 3, 1)

        self.modelTSRay.addItem('0', '0')
        self.modelTSRay.addItem('1', '1')
        self.modelTSRay.addItem('2', '2')

        self.modelPrintT.addItem('0', '0')
        self.modelPrintT.addItem('1', '1')
        self.modelPrintT.addItem('2', '2')

        self.modelPrintL.addItem('0', '0')
        self.modelPrintL.addItem('1', '1')
        self.modelPrintL.addItem('2', '2')

        self.frequ     = self.default['frequency']
        self.tsr       = self.default['idiver']
        self.printTemp = self.default['tempP']
        self.printLum  = self.default['intensity']
        model          = self.default['model']

        # Initialization

        self.lineEditFreq.setText(str(self.frequ))
        self.modelTSRay.setItem(str_model=str(self.tsr))
        self.modelPrintT.setItem(str_model=str(self.printTemp))
        self.modelPrintL.setItem(str_model=str(self.printLum))

        if model == 'dom':
            self.labelPrintL.show()
            self.comboBoxPrintL.show()
        else:
            self.labelPrintL.hide()
            self.comboBoxPrintL.hide()

        # Validator

        validatorFreq = QtPage.IntValidator(self.lineEditFreq, min=1)
        self.lineEditFreq.setValidator(validatorFreq)

        self.case.undoStartGlobal()


    def accept(self):
        """
        What to do when user clicks on 'OK'.
        """
        if self.lineEditFreq.validator().state == QValidator.Acceptable:
            self.result['frequency'] = int(self.lineEditFreq.text())
        self.result['idiver']    = int(self.comboBoxTSRay.currentText())
        self.result['tempP']     = int(self.comboBoxPrintT.currentText())
        self.result['intensity'] = int(self.comboBoxPrintL.currentText())

        QDialog.accept(self)


    def reject(self):
        """
        Method called when 'Cancel' button is clicked.
        """
        QDialog.reject(self)


    def get_result(self):
        """
        Method to get the result.
        """
        return self.result


    def tr(self, text):
        """
        Translation.
        """
        return text


#--------------------------------------------------------------------------------
# Main class
#--------------------------------------------------------------------------------

class ThermalRadiationView(QWidget, Ui_ThermalRadiationForm):
    """
    Class to open Thermal Scalar Transport Page.
    """
    def __init__(self, parent, case, tree):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_ThermalRadiationForm.__init__(self)
        self.setupUi(self)

        self.browser = tree
        self.case = case
        self.case.undoStopGlobal()
        self.mdl = ThermalRadiationModel(self.case)

        # Combo models

        self.modelRadModel   = QtPage.ComboModel(self.comboBoxRadModel, 3, 1)
        self.modelDirection  = QtPage.ComboModel(self.comboBoxQuadrature, 6, 1)
        self.modelAbsorption = QtPage.ComboModel(self.comboBoxAbsorption, 3, 1)

        self.modelRadModel.addItem("No radiative transfers", 'off')
        self.modelRadModel.addItem("Discrete ordinates method", 'dom')
        self.modelRadModel.addItem("P-1 Model", 'p-1')

        self.modelDirection.addItem("24 directions (S4)",   "1")
        self.modelDirection.addItem("48 directions (S6)",   "2")
        self.modelDirection.addItem("80 directions (S8)",   "3")
        self.modelDirection.addItem("32 directions (T2)",   "4")
        self.modelDirection.addItem("128 directions (T4)",  "5")
        self.modelDirection.addItem("8n^2 directions (Tn)", "6")

        # Connections

        self.connect(self.comboBoxRadModel,
                     SIGNAL("activated(const QString&)"),
                     self.slotRadiativeTransfer)
        self.connect(self.radioButtonOn,
                     SIGNAL("clicked()"),
                     self.slotStartRestart)
        self.connect(self.radioButtonOff,
                     SIGNAL("clicked()"),
                     self.slotStartRestart)
        self.connect(self.comboBoxQuadrature,
                     SIGNAL("activated(const QString&)"),
                     self.slotDirection)
        self.connect(self.lineEditNdirec,
                     SIGNAL("textChanged(const QString &)"),
                     self.slotNdirec)
        self.connect(self.comboBoxAbsorption,
                     SIGNAL("activated(const QString&)"),
                     self.slotTypeCoefficient)
        self.connect(self.lineEditCoeff,
                     SIGNAL("textChanged(const QString &)"),
                     self.slotAbsorptionCoefficient)
        self.connect(self.toolButtonAdvanced,
                     SIGNAL("clicked()"),
                     self.slotAdvancedOptions)

        # Validator

        validatorCoeff = QtPage.DoubleValidator(self.lineEditCoeff, min=0.0)
        self.lineEditCoeff.setValidator(validatorCoeff)

        validatorNdir = QtPage.IntValidator(self.lineEditNdirec, min=3)
        self.lineEditNdirec.setValidator(validatorNdir)

        self.modelAbsorption.addItem('constant',                   'constant')
        self.modelAbsorption.addItem('user subroutine (usray3)',   'variable')
        self.modelAbsorption.addItem('user law',                   'formula')
        self.modelAbsorption.addItem('H2O and CO2 mixing (Modak)', 'modak')

        from Pages.CoalCombustionModel import CoalCombustionModel
        if CoalCombustionModel(self.case).getCoalCombustionModel() != "off":
            self.modelAbsorption.disableItem(str_model='variable')
            self.modelAbsorption.enableItem(str_model='modak')
        else:
            self.modelAbsorption.disableItem(str_model='modak')
            self.modelAbsorption.enableItem(str_model='variable')

        del CoalCombustionModel

        self.modelAbsorption.disableItem(str_model='formula')

        # Initialization

        self.modelRadModel.setItem(str_model=self.mdl.getRadiativeModel())
        self.slotRadiativeTransfer()

        if self.mdl.getRestart() == 'on':
            self.radioButtonOn.setChecked(True)
            self.radioButtonOff.setChecked(False)
        else:
            self.radioButtonOn.setChecked(False)
            self.radioButtonOff.setChecked(True)

        value = self.mdl.getTypeCoeff()
        self.modelAbsorption.setItem(str_model=value)
        self.slotTypeCoefficient(self.modelAbsorption.dicoM2V[value])

        self.pushButtonCoeffFormula.setEnabled(False)

        self.lineEditCoeff.setText(str(self.mdl.getAbsorCoeff()))

        self.case.undoStartGlobal()


    @pyqtSignature("const QString &")
    def slotRadiativeTransfer(self):
        """
        """
        model = self.modelRadModel.dicoV2M[str(self.comboBoxRadModel.currentText())]
        self.mdl.setRadiativeModel(model)
        if model == 'off':
            self.frameOptions.hide()
            self.line.hide()
        else:
            self.frameOptions.show()
            self.line.show()

            if model == 'p-1':
                self.frameDirection.hide()
            elif model == 'dom':
                self.frameDirection.show()
                n = self.mdl.getQuadrature()
                self.modelDirection.setItem(str_model=str(n))

                if str(n) == "6":
                    self.lineEditNdirec.setEnabled(True)
                    self.lineEditNdirec.setText(str(self.mdl.getNbDir()))
                else:
                    self.lineEditNdirec.setEnabled(False)

        self.browser.configureTree(self.case)


    @pyqtSignature("")
    def slotStartRestart(self):
        """
        """
        if self.radioButtonOn.isChecked():
            self.mdl.setRestart("on")
        else:
            self.mdl.setRestart("off")


    @pyqtSignature("const QString &")
    def slotDirection(self, text):
        """
        """
        n = int(self.modelDirection.dicoV2M[str(text)])
        self.mdl.setQuadrature(n)

        if n == 6:
            self.lineEditNdirec.setEnabled(True)
            self.lineEditNdirec.setText(str(self.mdl.getNbDir()))
        else:
            self.lineEditNdirec.setEnabled(False)


    @pyqtSignature("const QString &")
    def slotNdirec(self, text):
        """
        """
        if self.sender().validator().state == QValidator.Acceptable:
            n = int(text)
            self.mdl.setNbDir(n)


    @pyqtSignature("const QString &")
    def slotTypeCoefficient(self, text):
        """
        """
        typeCoeff = self.modelAbsorption.dicoV2M[str(text)]
        self.mdl.setTypeCoeff(typeCoeff)

        if typeCoeff == 'constant':
            self.lineEditCoeff.setEnabled(True)
        elif typeCoeff == 'modak':
            self.lineEditCoeff.setDisabled(True)
        else:
            self.lineEditCoeff.setDisabled(True)


    @pyqtSignature("const QString &")
    def slotAbsorptionCoefficient(self, text):
        """
        """
        if self.sender().validator().state == QValidator.Acceptable:
            c  = float(text)
            self.mdl.setAbsorCoeff(c)


    @pyqtSignature("")
    def slotAdvancedOptions(self):
        """
        Ask one popup for advanced specifications
        """
        default = {}
        default['frequency'] = self.mdl.getFrequency()
        default['idiver']    = self.mdl.getTrs()
        default['tempP']     = self.mdl.getTemperatureListing()
        default['intensity'] = self.mdl.getIntensityResolution()
        default['model']     = self.mdl.getRadiativeModel()
        log.debug("slotAdvancedOptions -> %s" % str(default))

        dialog = ThermalRadiationAdvancedDialogView(self, self.case, default)
        if dialog.exec_():
            result = dialog.get_result()
            log.debug("slotAdvancedOptions -> %s" % str(result))
            self.mdl.setFrequency(result['frequency'])
            self.mdl.setTrs(result['idiver'])
            self.mdl.setTemperatureListing(result['tempP'])
            self.mdl.setIntensityResolution(result['intensity'])


    def tr(self, text):
        """
        Translation
        """
        return text


#-------------------------------------------------------------------------------
# Testing part
#-------------------------------------------------------------------------------


if __name__ == "__main__":
    pass


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
