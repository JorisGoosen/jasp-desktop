import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_helpbutton"
    when: windowShown

    HelpButton {
        id: helpButton
    }

    function test_helpbutton_visible() {
        verify(helpButton.visible, "HelpButton should be visible by default")
    }
}
