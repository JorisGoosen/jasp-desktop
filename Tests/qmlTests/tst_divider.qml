import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_divider"
    when: windowShown

    Divider {
        id: divider
    }

    function test_divider_visible() {
        verify(divider.visible, "Divider should be visible")
    }
}
