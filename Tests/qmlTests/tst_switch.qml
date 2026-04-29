import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_switch"
    when: windowShown

    Switch {
        id: switchComponent
        checked: false
    }

    function test_switch_initial_state() {
        compare(switchComponent.checked, false, "Switch should be unchecked by default")
    }

    function test_switch_toggle() {
        switchComponent.checked = true
        compare(switchComponent.checked, true, "Switch should be checked after toggle")
    }
}
