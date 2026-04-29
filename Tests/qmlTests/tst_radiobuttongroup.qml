import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_radiobuttongroup"
    when: windowShown

    RadioButtonGroup {
        id: group
    }

    RadioButton {
        id: rb1
        text: "1"
        ButtonGroup.group: group
    }

    RadioButton {
        id: rb2
        text: "2"
        ButtonGroup.group: group
    }

    function test_group_selection() {
        rb1.checked = true
        compare(group.checkedButton, rb1, "Group should track rb1")
        
        rb2.checked = true
        compare(group.checkedButton, rb2, "Group should track rb2")
        compare(rb1.checked, false, "rb1 should be unchecked when rb2 is selected")
    }
}
