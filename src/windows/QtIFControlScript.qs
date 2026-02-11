var quitInstaller = false;
var productName = installer.value("ProductName");
var installerTitle = installer.value("Title");
var legacyUninstallerGUIDs = [
"{17CF3E8D-A127-4491-BC24-28A345CECC03}",
"{BC3B0F4D-0C93-45BB-8054-1224C4525B1F}",
"{FD9E3BE1-F1CF-4FE9-8A2D-0E53C2493D36}",
"{4C8A423B-1E54-436C-A62C-B0B2E8C2E2DC}",
"{FBE15609-FF26-4B45-9DB8-4479A6139A63}",
"{B706703D-6935-4587-AFB2-4A02E5A01EA1}",
"{77532DB9-7D01-4C28-9F66-AD948AD9423E}",
"{87B437C2-A70A-4D3D-A10C-68AE148610DC}",
"{E31044CE-C97B-457B-A0F4-514CC87AA987}",
"{354B5FEA-D2FB-4908-9B4C-D137DB69CE83}",
"{64D76CDC-9F69-4C07-8454-FEB4E409556D}",
"{2CE18E8F-9ED3-46D5-867B-6DA0494838BA}",
"{841E427A-041D-4C69-B3B3-117916D46093}",
"{D29AA00A-30D7-4060-A466-DDC80A1A6588}",
"{A92A45FE-1EB0-4A13-A212-D96C0FC09862}",
"{2D89D363-ED62-4FB5-BCFF-3A6D7DBBAB44}"
];

function Controller()
{
    let legacyUninstallerVersion = "";
    for (const guid of legacyUninstallerGUIDs) {
        const key = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + guid + "\\DisplayVersion";
        console.log("Checking if legacy uninstaller exists for key: " + key);
        const version = installer.value(key, "", QSettings.NativeFormat);
        if (version) {
            console.log("Found legacy uninstaller with version: " + version);
            legacyUninstallerVersion = version;
            break;
        }
    }
    if (legacyUninstallerVersion) {
        var result = QMessageBox.warning("legacy_installer.question", installerTitle, "An old " + productName + " installation was found (Version " + legacyUninstallerVersion + "). It is strongly recommended to uninstall " + productName + " first before installing the new version.", QMessageBox.Abort | QMessageBox.Ignore);
        if (result == QMessageBox.Abort) {
            quitInstaller = true;
            console.log("User decided to quit installer");
            installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
            installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
            installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
            installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
            installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
            installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
            gui.clickButton(buttons.NextButton);
        }
    }
}

Controller.prototype.FinishedPageCallback = function()
{
    var widget = gui.currentPageWidget();
    widget.RunItCheckBox.checked = false;
    widget.RunItCheckBox.visible = false;
    if (quitInstaller) {
        widget.title = "";
        widget.MessageLabel.setText("<font color='red'>Installation was canceled.</font>");
    }
}
