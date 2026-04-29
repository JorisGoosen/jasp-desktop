import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_form"
    when: windowShown

    Form {
        id: form
    }

    function test_form_visible() {
        verify(form.visible, "Form should be visible")
    }
}
