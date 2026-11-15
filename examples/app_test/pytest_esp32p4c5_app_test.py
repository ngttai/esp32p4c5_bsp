import pytest
from pytest_embedded import Dut


@pytest.mark.ksdiy_p4c5
def test_example_app_test(dut: Dut) -> None:
    dut.expect_exact('app test ready')
