import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_button"
    when: windowShown

    Button {
        id: button
        text: "Test Button"
    }

    function test_button_visible() {
        verify(button.visible, "Button should be visible by default")
    }

    function test_button_text() {
        compare(button.text, "Test Button", "Button text should match")
    }
}
