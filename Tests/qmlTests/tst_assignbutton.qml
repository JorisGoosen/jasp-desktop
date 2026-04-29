import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_assignbutton"
    when: windowShown

    AssignButton {
        id: assignButton
        text: "Assign"
    }

    function test_assignbutton_visible() {
        verify(assignButton.visible, "AssignButton should be visible by default")
    }

    function test_assignbutton_text() {
        compare(assignButton.text, "Assign", "AssignButton text should match")
    }
}
