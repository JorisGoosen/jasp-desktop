import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_menubutton"
    when: windowShown

    MenuButton {
        id: menuButton
        text: "Menu"
    }

    function test_menubutton_visible() {
        verify(menuButton.visible, "MenuButton should be visible by default")
    }

    function test_menubutton_text() {
        compare(menuButton.text, "Menu", "MenuButton text should match")
    }
}
