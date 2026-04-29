import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_crossbutton"
    when: windowShown

    CrossButton {
        id: crossButton
    }

    function test_crossbutton_visible() {
        verify(crossButton.visible, "CrossButton should be visible by default")
    }
}
