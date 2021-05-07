Contributing
============

libvfn accepts contributions through GitHub Pull Requests. Contributions may
also be sent as properly formatted patches to the project maintainer by email.


Style
-----

libvfn largely follows the `Linux Kernel Coding Style
<https://www.kernel.org/doc/html/latest/process/coding-style.html>`__.

Check your patches prior to submission using ``scripts/checkpatch.pl``.

If you are using Vim, see the `.vimrc <.vimrc>`__ file.


Sparse
------

If ``sparse`` is installed, you can run static analysis using

.. code::

	meson compile -C build sparse


Developer Certificate of Origin
-------------------------------

All contributions to libvfn must be signed off by you. Signing off on your
patch certifies that you wrote the patch or otherwise have the right to pass it
on to an open source project. By signing off you agree to the Developer
Certificate of Origin as shown below (from `developercertificate.org
<http://developercertificate.org>`__:

::

	Developer Certificate of Origin
	Version 1.1

	Copyright (C) 2004, 2006 The Linux Foundation and its contributors.

	Everyone is permitted to copy and distribute verbatim copies of this
	license document, but changing it is not allowed.


	Developer's Certificate of Origin 1.1

	By making a contribution to this project, I certify that:

	(a) The contribution was created in whole or in part by me and I
	    have the right to submit it under the open source license
	    indicated in the file; or

	(b) The contribution is based upon previous work that, to the best
	    of my knowledge, is covered under an appropriate open source
	    license and I have the right under that license to submit that
	    work with modifications, whether created in whole or in part
	    by me, under the same open source license (unless I am
	    permitted to submit under a different license), as indicated
	    in the file; or

	(c) The contribution was provided directly to me by some other
	    person who certified (a), (b) or (c) and I have not modified
	    it.

	(d) I understand and agree that this project and the contribution
	    are public and that a record of the contribution (including all
	    personal information I submit with it, including my sign-off) is
	    maintained indefinitely and may be redistributed consistent with
	    this project or the open source license(s) involved.


To sign off on your contributions, add the following line in all patches:

::

	Signed-off-by: Real Name <email@address>


**NOTE** It is imperative that you use your real name. Anonymous contributions
or contributions done under a pseudonym will not be accepted.
