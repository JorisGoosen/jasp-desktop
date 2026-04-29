import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_checkbox"
    when: windowShown

    CheckBox {
        id: checkBox
        checked: false
    }

    function test_checkbox_initial_state() {
        compare(checkBox.checked, false, "CheckBox should be unchecked by default")
    }

    function test_checkbox_toggle() {
        checkBox.checked = true
        compare(checkBox.checked, true, "CheckBox should be checked after toggle")
    }
}
