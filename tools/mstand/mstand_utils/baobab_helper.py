# -*- coding: utf-8 -*-


def get_block_position(request):
    block_position = {}
    if request.IsA("TBaobabProperties"):
        import tamus
        joiners = request.BaobabAllTrees()
        if joiners:
            marks = tamus.check_rules_multiple_joiners_merged({}, joiners)
            markers = ["top_blocks", "main_blocks", "parallel_blocks"]
            for marker in markers:
                for i, block in enumerate(marks.get_blocks(marker)):
                    block_position[block.id] = i
    return block_position


def get_block_attrs(request, block):
    if request.IsA("TBaobabProperties"):
        joiners = request.BaobabAllTrees()
        if joiners:
            b_block = request.FindBaobabBlockByRALibBlock(block)
            if b_block:
                return dict(
                    b_block.attrs,
                    block_id=b_block.id,
                    parent_name=b_block.parent.name,
                )


def get_block_id(request, block):
    if request.IsA("TBaobabProperties"):
        joiners = request.BaobabAllTrees()
        if joiners:
            b_block = request.FindBaobabBlockByRALibBlock(block)
            if b_block:
                return b_block.id
    return ""


def get_adv_type(request, block):
    adv_type = "guarantee"
    b_block = request.FindBaobabBlockByRALibBlock(block)
    if b_block:
        import tamus
        joiners = request.BaobabAllTrees()
        marks = tamus.check_rules_multiple_joiners_merged({}, joiners)
        if any(marks.has_marker_in_block(b_block, marker) for marker in ("premium", "sticky")):
            adv_type = "dir"
    return adv_type


class BaobabHelper:
    def __init__(self, request):
        self.request = request
        self.joiners = request.BaobabAllTrees()
        self.inversed_weak_ties = self._get_inverse_weak_ties()

    def get_click_attrs(self, click):
        if self.request.IsA("TBaobabProperties") and click.IsA("TBaobabClickProperties"):
            if self.joiners:
                click_block = click.BaobabBlock(self.joiners)
                if click_block:
                    return self._get_attrs(click_block)

    def get_tech_attrs(self, tech):
        if self.request.IsA("TBaobabProperties"):
            if self.joiners:
                for joiner in self.joiners:
                    baobabTech = tech.BaobabTech(joiner)
                    if baobabTech:
                        if joiner.get_show() is not None:
                            tech_block = joiner.get_show().tree.get_block_by_id(baobabTech.block_id)
                            if tech_block:
                                return self._get_attrs(tech_block)

    @staticmethod
    def _get_root_id(block):
        current = block
        while current.parent:
            current = current.parent
        return current.id

    def _get_inverse_weak_ties(self):
        # noinspection PyUnresolvedReferences,PyPackageRequirements
        import baobab

        weak_ties = baobab.common.build_weak_ties(self.joiners)
        inversed_weak_ties = {}
        for joiner in self.joiners:
            show = joiner.get_show()
            if show is not None:
                root_id = show.tree.root.id
                for block in baobab.common.dfs_iterator(show.tree.root):
                    if (root_id, block.id) not in inversed_weak_ties or block.parent:
                        inversed_weak_ties[(root_id, block.id)] = (root_id, block.parent)
                    weak_ties_for_block = weak_ties.get_weak_ties(block.id)
                    for child_block in weak_ties_for_block:
                        inversed_weak_ties[(self._get_root_id(child_block), child_block.id)] = (root_id, block)

        return inversed_weak_ties

    def _get_result_id_and_path(self, event_block):
        result = []
        current_root_id = self._get_root_id(event_block)
        current_block = event_block
        while current_block:
            result.append(current_block.name)
            if current_block.name == "$result":
                return current_block.id, "/".join(reversed(result))
            current_root_id, current_block = self.inversed_weak_ties[(current_root_id, current_block.id)]
        return None, "/".join(reversed(result))

    def _get_attrs(self, event_block):
        result_block_id, path = self._get_result_id_and_path(event_block)
        if result_block_id:
            return {
                "path": path,
                "block_id": result_block_id,
            }
        else:
            return {"path": path}
