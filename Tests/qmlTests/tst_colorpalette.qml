import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_colorpalette"
    when: windowShown

    ColorPalette {
        id: palette
    }

    function test_colorpalette_visible() {
        verify(palette.visible, "ColorPalette should be visible")
    }
}
