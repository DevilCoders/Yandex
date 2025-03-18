INSERT INTO public.permissions(uid, role, node)
VALUES
  (393530940, 'admin', '*'), -- yndx-ddemidov
  (539446614, 'admin', '*'), -- yndx-ddemidov-manager
  (413252796, 'admin', '*'), -- yndx.sotskov
  (598686045, 'admin', '*'), -- yndx.mkhardin
  (609573294, 'admin', '*'), -- yndx-aab-admin-solovyev
  (45974892, 'admin', '*'),  -- lawyard
  (570377437, 'admin', '*'), -- lexx-evd
  (544175550, 'admin', '*'), -- yndx.simonovin
  (592498701, 'admin', '*'), -- tzapil
  (134874470, 'admin', '*')  -- denis.chernilevskiy
ON CONFLICT (uid, node) DO UPDATE SET role = 'admin';