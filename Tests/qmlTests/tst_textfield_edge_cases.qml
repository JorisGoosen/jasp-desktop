import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_textfield_edge_cases"
    when: windowShown

    TextField {
        id: textField
        text: ""
    }

    function test_textfield_empty() {
        compare(textField.text, "", "TextField should handle empty string")
    }

    function test_textfield_long_text() {
        let longStr = "This is a very long string that should be handled correctly by the TextField component without crashing or causing issues."
        textField.text = longStr
        compare(textField.text, longStr, "TextField should handle long strings")
    }
}
