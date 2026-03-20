.PHONY: srpm

srpm:
	dnf install -y rpm-build rpmdevtools
	spectool -g -C $(outdir) .packages/fedora/muslimtify.spec
	rpmbuild -bs .packages/fedora/muslimtify.spec \
		--define "_sourcedir $(outdir)" \
		--define "_srcrpmdir $(outdir)"
