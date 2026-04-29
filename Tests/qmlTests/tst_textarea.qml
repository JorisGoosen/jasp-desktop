import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_textarea"
    when: windowShown

    TextArea {
        id: textArea
        text: "Hello World"
    }

    function test_textarea_text() {
        compare(textArea.text, "Hello World", "TextArea text should match initial value")
    }

    function test_textarea_update() {
        textArea.text = "New Text"
        compare(textArea.text, "New Text", "TextArea text should update")
    }
}
