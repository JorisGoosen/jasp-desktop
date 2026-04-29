import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_rectangularbutton"
    when: windowShown

    RectangularButton {
        id: rectangularButton
        text: "Rectangular"
    }

    function test_rectangularbutton_visible() {
        verify(rectangularButton.visible, "RectangularButton should be visible by default")
    }

    function test_rectangularbutton_text() {
        compare(rectangularButton.text, "Rectangular", "RectangularButton text should match")
    }
}
