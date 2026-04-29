import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_jagstextarea"
    when: windowShown

    JAGSTextArea {
        id: jagsTextArea
        text: "jags code"
    }

    function test_jagstextarea_text() {
        compare(jagsTextArea.text, "jags code", "JAGSTextArea text should match initial value")
    }

    function test_jagstextarea_update() {
        jagsTextArea.text = "updated jags"
        compare(jagsTextArea.text, "updated jags", "JAGSTextArea text should update")
    }
}
