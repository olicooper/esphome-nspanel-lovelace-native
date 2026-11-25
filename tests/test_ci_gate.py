"""Check that the required CI gate rejects failed or skipped jobs."""

import subprocess
from pathlib import Path

import yaml


def test_required_gate_requires_both_jobs_to_succeed():
    workflow = yaml.load(
        (Path(__file__).resolve().parents[1] / ".github/workflows/build.yaml").read_text(),
        Loader=yaml.BaseLoader,
    )
    gate = workflow["jobs"]["required"]
    assert set(gate["needs"]) == {"build", "unittest"}
    assert gate["if"] == "${{ always() }}"
    step = gate["steps"][0]
    for build, tests, expected in [
        ("success", "success", 0),
        ("failure", "success", 1),
        ("cancelled", "success", 1),
        ("skipped", "success", 1),
        ("success", "failure", 1),
        ("success", "skipped", 1),
    ]:
        result = subprocess.run(
            ["bash", "-c", step["run"]],
            env={"BUILD_RESULT": build, "TEST_RESULT": tests},
            check=False,
        )
        assert result.returncode == expected, (build, tests)
