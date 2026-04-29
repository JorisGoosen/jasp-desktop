import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_roundedbutton"
    when: windowShown

    RoundedButton {
        id: roundedButton
        text: "Rounded"
    }

    function test_roundedbutton_visible() {
        verify(roundedButton.visible, "RoundedButton should be visible by default")
    }

    function test_roundedbutton_text() {
        compare(roundedButton.text, "Rounded", "RoundedButton text should match")
    }
}
