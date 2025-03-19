;; -*-emacs-lisp-*-
;;
;; Emacs startup file, e.g.  /etc/emacs/site-start.d/50config-kp-space-backup.el
;; for the Debian config-kp-space-backup package
;;
;; Originally contributed by Nils Naumann <naumann@unileoben.ac.at>
;; Modified by Dirk Eddelbuettel <edd@debian.org>
;; Adapted for dh-make by Jim Van Zandt <jrv@debian.org>

;; The config-kp-space-backup package follows the Debian/GNU Linux 'emacsen' policy and
;; byte-compiles its elisp files for each 'emacs flavor' (emacs19,
;; xemacs19, emacs20, xemacs20...).  The compiled code is then
;; installed in a subdirectory of the respective site-lisp directory.
;; We have to add this to the load-path:
(let ((package-dir (concat "/usr/share/"
                           (symbol-name flavor)
                           "/site-lisp/config-kp-space-backup")))
;; If package-dir does not exist, the config-kp-space-backup package must have
;; removed but not purged, and we should skip the setup.
  (when (file-directory-p package-dir)
    (setq load-path (cons package-dir load-path))
    (autoload 'config-kp-space-backup-mode "config-kp-space-backup-mode"
      "Major mode for editing config-kp-space-backup files." t)
    (add-to-list 'auto-mode-alist '("\\.config-kp-space-backup$" . config-kp-space-backup-mode))))

