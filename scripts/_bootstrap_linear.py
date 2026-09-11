"""One-shot Linear project bootstrap. Reads LINEAR_API_KEY from env or ~/.tapps-operator.env. Never prints the key."""
from __future__ import annotations

import json
import os
import pathlib
import urllib.error
import urllib.request

API = "https://api.linear.app/graphql"
TEAM_NAME = "TappsCodingAgents"
TEAM_KEY = "TAP"
PROJECT_NAME = "Core2"
DESCRIPTION = (
    "Engineering work for the Core2 repository (github.com/wtthornton/Core2)."
)


def load_key() -> str | None:
    key = os.environ.get("LINEAR_API_KEY", "").strip()
    if key:
        return key
    for path in (
        pathlib.Path.home() / ".tapps-operator.env",
        pathlib.Path.home() / ".config" / "claude-agent" / "linear.env",
        pathlib.Path.home() / ".linear.env",
    ):
        if not path.is_file():
            continue
        for raw in path.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("export "):
                line = line[7:].strip()
            if "=" not in line:
                continue
            name, value = line.split("=", 1)
            name = name.strip()
            value = value.strip().strip("'").strip('"')
            if name == "LINEAR_API_KEY" and value:
                return value
    return None


def gql(key: str, query: str, variables: dict | None = None) -> dict:
    payload = json.dumps({"query": query, "variables": variables or {}}).encode()
    req = urllib.request.Request(
        API,
        data=payload,
        headers={
            "Authorization": key,
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            body = json.loads(resp.read().decode())
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode(errors="replace")
        raise SystemExit(f"Linear HTTP {exc.code}: {detail[:500]}") from exc
    if body.get("errors"):
        messages = "; ".join(
            str(err.get("message", err)) for err in body["errors"]
        )
        raise SystemExit(f"Linear GraphQL error: {messages}")
    return body["data"]


def main() -> None:
    key = load_key()
    if not key:
        print("STATUS=missing_api_key")
        return

    data = gql(
        key,
        """
        query TeamsAndProjects {
          teams {
            nodes { id name key }
          }
          projects(filter: { name: { eqIgnoreCase: "Core2" } }) {
            nodes { id name url slugId }
          }
        }
        """,
    )
    teams = data["teams"]["nodes"]
    team = next(
        (t for t in teams if t["key"] == TEAM_KEY or t["name"] == TEAM_NAME),
        None,
    )
    if not team:
        names = ", ".join(f"{t['key']}:{t['name']}" for t in teams)
        raise SystemExit(f"Team {TEAM_NAME}/{TEAM_KEY} not found. Available: {names}")

    existing = data["projects"]["nodes"]
    if existing:
        project = existing[0]
        print("STATUS=exists")
        print(f"TEAM_ID={team['id']}")
        print(f"TEAM_NAME={team['name']}")
        print(f"TEAM_KEY={team['key']}")
        print(f"PROJECT_ID={project['id']}")
        print(f"PROJECT_NAME={project['name']}")
        print(f"PROJECT_URL={project.get('url', '')}")
        print(f"PROJECT_SLUG={project.get('slugId', '')}")
        return

    created = gql(
        key,
        """
        mutation CreateProject($input: ProjectCreateInput!) {
          projectCreate(input: $input) {
            success
            project { id name url slugId }
          }
        }
        """,
        {
            "input": {
                "name": PROJECT_NAME,
                "teamIds": [team["id"]],
                "description": DESCRIPTION,
            }
        },
    )["projectCreate"]
    if not created.get("success"):
        raise SystemExit("projectCreate returned success=false")
    project = created["project"]
    print("STATUS=created")
    print(f"TEAM_ID={team['id']}")
    print(f"TEAM_NAME={team['name']}")
    print(f"TEAM_KEY={team['key']}")
    print(f"PROJECT_ID={project['id']}")
    print(f"PROJECT_NAME={project['name']}")
    print(f"PROJECT_URL={project.get('url', '')}")
    print(f"PROJECT_SLUG={project.get('slugId', '')}")


if __name__ == "__main__":
    main()
