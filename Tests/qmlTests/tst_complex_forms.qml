import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_complex_forms"
    when: windowShown

    VariablesForm {
        id: variablesForm
    }

    FactorsForm {
        id: factorsForm
    }

    function test_complex_forms_exist() {
        verify(variablesForm, "VariablesForm should exist")
        verify(factorsForm, "FactorsForm should exist")
    }
}
