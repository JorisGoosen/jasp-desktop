import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_radiobutton"
    when: windowShown

    RadioButton {
        id: radioButton
        text: "Option 1"
    }

    function test_radiobutton_visible() {
        verify(radioButton.visible, "RadioButton should be visible")
    }

    function test_radiobutton_text() {
        compare(radioButton.text, "Option 1", "RadioButton text should match")
    }
}
