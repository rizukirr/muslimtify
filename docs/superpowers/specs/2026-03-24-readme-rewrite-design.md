# README Rewrite Design

## Goal

Rewrite `README.md` into a concise, production-ready user document focused on
installing, configuring, and operating Muslimtify on Linux and Windows.

## Audience

This README should optimize for end users, not contributors reading the source
for the first time.

It should answer:
- what Muslimtify does
- how to install it
- what a user must run to make it work after install
- where configuration lives
- how to configure it with CLI commands or manual JSON edits
- where to go for troubleshooting, contribution, license, and support

## Scope

This rewrite applies only to `README.md`.

It does not change:
- CLI behavior
- packaging behavior
- install scripts
- detailed developer docs under `docs/`

## Content Priorities

The new README should be shorter and more operational than the current version.

Keep:
- short cross-platform overview
- installation methods for package managers and source installs
- required `muslimtify daemon install` step in every install flow
- configuration paths for Linux and Windows
- brief explanation of manual JSON editing and CLI-based configuration
- collapsible default `config.json`
- concise troubleshooting
- contribution, license, and support sections

Remove or compress:
- large feature-marketing language
- big command catalogs
- large sample output blocks
- implementation-detail-heavy sections
- repeated explanations

## Information Architecture

The README should be reorganized into these sections:

1. Overview
2. Installation
3. Configuration
4. Troubleshooting
5. Contributing
6. License
7. Support

No standalone "How It Works" section.
No standalone "Quick Start" section.
No standalone daemon/background implementation section.

## Section Requirements

### Overview

Keep this short. It should explain:
- Muslimtify calculates prayer times locally
- configuration is stored in the user profile
- periodic background checks are used to deliver notifications
- Linux and Windows are both supported

This section should not turn into a feature list or a command reference.

### Installation

Include all current user-facing install options:
- Arch Linux (AUR)
- Fedora (COPR)
- Debian/Ubuntu (PPA)
- Linux source install
- Windows source install

Every install flow should end with `muslimtify daemon install` so users who
copy-paste the commands leave with a working background setup.

The Windows install subsection should clearly note:
- Windows support is currently beta
- users should expect rough edges and occasional breaking changes

### Configuration

This section should explain both supported ways to configure the app:
- command line
- manual JSON editing

Include:
- Linux config path: `~/.config/muslimtify/config.json`
- Windows config path: `%APPDATA%\muslimtify\config.json`
- Linux cache path: `~/.cache/muslimtify`
- Windows cache path: `%LOCALAPPDATA%\muslimtify`

Include a concise CLI-based configuration block covering only the most useful
setup commands, such as:
- `muslimtify location auto`
- `muslimtify location set <lat> <lon>`
- `muslimtify reminder all 30,15,5`
- `muslimtify config show`
- `muslimtify config validate`
- `muslimtify config reset`

Also include a short note that manual JSON editing is useful for precise
control.

Add the default `config.json` inside a collapsible `<details>` block.

### Troubleshooting

Keep this short and practical.

Cover:
- notifications not appearing
- location detection issues
- config validation/reset
- note that Windows toast delivery can be affected by local system settings

### Contributing

Replace the existing development-oriented section with a short pointer to
`CONTRIBUTING.md`.

### License

State MIT clearly and point to the repository files.

### Support

Keep support options concise:
- GitHub issues/discussions
- optional support/donation link if already present

## Tone

The new README should be:
- concise
- direct
- production-ready
- user-focused

Avoid:
- warning banners at the top
- emoticons or decorative symbols
- exaggerated marketing language
- unnecessarily long examples

## Expected Outcome

After the rewrite, a new user should be able to:
- choose an install method
- run the required install commands
- know where the config file lives
- configure Muslimtify through commands or JSON
- understand where to look when something goes wrong
