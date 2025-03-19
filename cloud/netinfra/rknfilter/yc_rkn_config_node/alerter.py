# from collections import defaultdict
# from concurrent.futures import ThreadPoolExecutor, as_completed


# def find_intersections(whitelist, xml_content):
#     intersections = defaultdict(lambda: defaultdict(int))
#     run_in_parallel(
#         find_intersections_for_whitelist_entry,
#         whitelist,
#         xml_content=xml_content,
#         intersections=intersections,
#     )
#     return intersections


# def find_intersections_for_whitelist_entry(whitelist_entry, xml_content, intersections):
#     run_in_parallel(
#         verify_that_content_entry_matches_whitelist_entry,
#         xml_content,
#         whitelist_entry=whitelist_entry,
#         intersections=intersections,
#     )


# def verify_that_content_entry_matches_whitelist_entry(
#     xml_content_entry, whitelist_entry, intersections
# ):
#     for wl_ip_prefix_object in whitelist_entry.ip_prefix_object:
#         for ip_address_object in xml_content_entry.ip_address_objects:
#             if ip_address_object in wl_ip_prefix_object:
#                 intersections[whitelist_entry.asn_name][
#                     str(wl_ip_prefix_object)
#                 ].append(str(ip_address_object))
#     for ip_prefix_object in xml_content_entry.ip_address_objects:
#         if ip_prefix_object in wl_ip_prefix_object:
#             intersections[whitelist_entry.asn_name][str(wl_ip_prefix_object)].append(
#                 str(ip_prefix_object)
#             )
